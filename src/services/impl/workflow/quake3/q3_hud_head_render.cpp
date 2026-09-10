#include "services/interfaces/workflow/quake3/q3_hud_head_render.hpp"

#include <SDL3/SDL.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <algorithm>
#include <cmath>
#include <cstring>

namespace sdl3cpp::services::impl {

HeadAngles UpdateHeadSway(HeadSwayState& state, uint64_t nowMs) {
    if (nowMs >= state.swayEndMs) {
        // Pick new random target angles
        state.swayStartYaw = state.swayEndYaw;
        state.swayStartPitch = state.swayEndPitch;
        state.swayStartMs = state.swayEndMs;
        state.swayEndMs = nowMs + 100 + static_cast<uint64_t>(SDL_rand(2000));

        const float ryaw =
            (static_cast<float>(SDL_rand(1000)) / 1000.f - 0.5f) * 2.f;
        const float rpitch =
            (static_cast<float>(SDL_rand(1000)) / 1000.f - 0.5f) * 2.f;
        state.swayEndYaw = glm::radians(180.f + 20.f * ryaw);
        state.swayEndPitch = glm::radians(5.f * rpitch);
    }
    if (state.swayStartMs == 0) state.swayStartMs = nowMs;

    float frac = 0.f;
    if (state.swayEndMs > state.swayStartMs) {
        frac = static_cast<float>(nowMs - state.swayStartMs) /
               static_cast<float>(state.swayEndMs - state.swayStartMs);
        frac = std::min(1.f, std::max(0.f, frac));
        frac = frac * frac * (3.f - 2.f * frac);  // smoothstep
    }

    HeadAngles out;
    out.yaw = state.swayStartYaw +
             (state.swayEndYaw - state.swayStartYaw) * frac;
    out.pitch = state.swayStartPitch +
               (state.swayEndPitch - state.swayStartPitch) * frac;
    return out;
}

glm::mat4 BuildHeadPortraitMvp(const HeadAngles& angles, float camDist) {
    const glm::vec3 camPos(
        std::sin(angles.yaw) * std::cos(angles.pitch) * camDist,
        -std::sin(angles.pitch) * camDist,
        std::cos(angles.yaw) * std::cos(angles.pitch) * camDist);

    const glm::mat4 view =
        glm::lookAt(camPos, glm::vec3(0.0f, 0.04f, 0.0f),
                   glm::vec3(0.0f, 1.0f, 0.0f));
    // Q3A uses FOV=30° for head (tan(15°)=0.268); keep 30° here too.
    const glm::mat4 proj = glm::perspective(glm::radians(30.0f),
                                            1.0f /*square aspect*/, 0.01f,
                                            10.0f);
    const glm::mat4 model(1.0f);  // head at world origin
    return proj * view * model;
}

rendering::FragmentUniformData DefaultHeadPortraitLighting() {
    rendering::FragmentUniformData fu{};
    fu.material[0] = 0.7f;  // roughness
    fu.material[1] = 0.0f;  // metallic
    // Soft front-right key light
    fu.light_dir[0] = 0.5f;
    fu.light_dir[1] = -0.7f;
    fu.light_dir[2] = -0.5f;
    fu.light_color[0] = 1.2f;
    fu.light_color[1] = 1.1f;
    fu.light_color[2] = 1.0f;
    fu.ambient[0] = fu.ambient[1] = fu.ambient[2] = 0.35f;
    return fu;
}

HeadRenderTargets CreateHeadRenderTargets(SDL_GPUDevice* device,
                                          SDL_Window* window, int size) {
    HeadRenderTargets out;

    SDL_GPUTextureCreateInfo ci{};
    ci.type = SDL_GPU_TEXTURETYPE_2D;
    ci.width = size;
    ci.height = size;
    ci.layer_count_or_depth = 1;
    ci.num_levels = 1;

    // Must match the swapchain format so gpu_pipeline_textured (compiled
    // for the swapchain) can render into this target without a
    // format-mismatch error.
    const SDL_GPUTextureFormat scFmt =
        window ? SDL_GetGPUSwapchainTextureFormat(device, window)
               : SDL_GPU_TEXTUREFORMAT_B8G8R8A8_UNORM;

    ci.format = scFmt;
    ci.usage =
        SDL_GPU_TEXTUREUSAGE_COLOR_TARGET | SDL_GPU_TEXTUREUSAGE_SAMPLER;
    out.color = SDL_CreateGPUTexture(device, &ci);

    ci.format = SDL_GPU_TEXTUREFORMAT_D32_FLOAT;
    ci.usage = SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET;
    out.depth = SDL_CreateGPUTexture(device, &ci);

    out.ready = out.color && out.depth;
    return out;
}

void DrawHeadMd3(const std::string& prefix, const glm::mat4& mvp,
                 const glm::mat4& model,
                 const rendering::FragmentUniformData& fu,
                 SDL_GPURenderPass* pass, SDL_GPUCommandBuffer* cmd,
                 WorkflowContext& context) {
    const int nSurfs = context.Get<int>("q3.md3." + prefix + "_num_surfs", 0);
    if (nSurfs <= 0) return;

    rendering::VertexUniformData vu{};
    std::memcpy(vu.mvp, glm::value_ptr(mvp), sizeof(float) * 16);
    std::memcpy(vu.model_mat, glm::value_ptr(model), sizeof(float) * 16);
    vu.normal[1] = 1.0f;
    vu.uv_scale[0] = 1.0f;
    vu.uv_scale[1] = 1.0f;

    // Identity shadow VP — no shadows in portrait
    const glm::mat4 noShadow(1.0f);
    std::memcpy(vu.shadow_vp, glm::value_ptr(noShadow), sizeof(float) * 16);

    for (int s = 0; s < nSurfs; ++s) {
        const std::string sk =
            "q3.md3." + prefix + "_surf" + std::to_string(s);
        auto* vb = context.Get<SDL_GPUBuffer*>(sk + "_f0_vb", nullptr);
        auto* ib = context.Get<SDL_GPUBuffer*>(sk + "_ib", nullptr);
        const int nIdx = context.Get<int>(sk + "_num_idx", 0);
        if (!vb || !ib || nIdx <= 0) continue;

        auto* tex = context.Get<SDL_GPUTexture*>(sk + "_tex", nullptr);
        auto* samp = context.Get<SDL_GPUSampler*>(sk + "_samp", nullptr);
        if (!tex || !samp) continue;

        // Bind albedo twice — slot 1 used as shadow map placeholder
        SDL_GPUTextureSamplerBinding bindings[2] = {{tex, samp}, {tex, samp}};
        SDL_BindGPUFragmentSamplers(pass, 0, bindings, 2);

        SDL_GPUBufferBinding vbb{vb, 0};
        SDL_BindGPUVertexBuffers(pass, 0, &vbb, 1);
        SDL_GPUBufferBinding ibb{ib, 0};
        SDL_BindGPUIndexBuffer(pass, &ibb, SDL_GPU_INDEXELEMENTSIZE_16BIT);
        SDL_PushGPUVertexUniformData(cmd, 0, &vu, sizeof(vu));
        SDL_PushGPUFragmentUniformData(cmd, 0, &fu, sizeof(fu));
        SDL_DrawGPUIndexedPrimitives(pass, (uint32_t)nIdx, 1, 0, 0, 0);
    }
}

bool RenderHeadPortraitPass(SDL_GPUCommandBuffer* cmd,
                            const HeadRenderTargets& targets,
                            SDL_GPUGraphicsPipeline* pipeline,
                            const glm::mat4& mvp, const glm::mat4& model,
                            const rendering::FragmentUniformData& fu,
                            WorkflowContext& context, int size) {
    SDL_GPUColorTargetInfo cti{};
    cti.texture = targets.color;
    cti.clear_color = {0.05f, 0.05f, 0.06f, 1.0f};  // near-black background
    cti.load_op = SDL_GPU_LOADOP_CLEAR;
    cti.store_op = SDL_GPU_STOREOP_STORE;

    SDL_GPUDepthStencilTargetInfo dti{};
    dti.texture = targets.depth;
    dti.clear_depth = 1.0f;
    dti.load_op = SDL_GPU_LOADOP_CLEAR;
    dti.store_op = SDL_GPU_STOREOP_DONT_CARE;

    SDL_GPURenderPass* pass = SDL_BeginGPURenderPass(cmd, &cti, 1, &dti);
    if (!pass) return false;

    SDL_GPUViewport vp{};
    vp.x = 0;
    vp.y = 0;
    vp.w = static_cast<float>(size);
    vp.h = static_cast<float>(size);
    vp.min_depth = 0.0f;
    vp.max_depth = 1.0f;
    SDL_SetGPUViewport(pass, &vp);

    SDL_BindGPUGraphicsPipeline(pass, pipeline);
    DrawHeadMd3("head", mvp, model, fu, pass, cmd, context);

    SDL_EndGPURenderPass(pass);
    return true;
}

}  // namespace sdl3cpp::services::impl
