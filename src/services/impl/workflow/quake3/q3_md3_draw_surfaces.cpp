#include "services/interfaces/workflow/quake3/q3_md3_draw_surfaces.hpp"

#include <glm/gtc/type_ptr.hpp>

#include <cstring>

namespace sdl3cpp::services::impl {

void DrawMd3Surfaces(const std::string& prefix, int frame,
                     const glm::mat4& model, const glm::mat4& view,
                     const glm::mat4& proj, const glm::vec3& camPos,
                     const glm::mat4& shadowVP,
                     const rendering::FragmentUniformData& fu,
                     SDL_GPURenderPass* pass, SDL_GPUCommandBuffer* cmd,
                     SDL_GPUTexture* shadowTex, SDL_GPUSampler* shadowSamp,
                     WorkflowContext& context) {
    const int nSurfs =
        context.Get<int>("q3.md3." + prefix + "_num_surfs", 0);
    if (nSurfs <= 0) return;

    const glm::mat4 mvp = proj * view * model;
    rendering::VertexUniformData vu = {};
    std::memcpy(vu.mvp, glm::value_ptr(mvp), sizeof(float) * 16);
    std::memcpy(vu.model_mat, glm::value_ptr(model), sizeof(float) * 16);
    vu.normal[1]   = 1.0f;
    vu.uv_scale[0] = 1.0f;
    vu.uv_scale[1] = 1.0f;
    vu.camera_pos[0] = camPos.x;
    vu.camera_pos[1] = camPos.y;
    vu.camera_pos[2] = camPos.z;
    std::memcpy(vu.shadow_vp, glm::value_ptr(shadowVP), sizeof(float) * 16);

    for (int s = 0; s < nSurfs; ++s) {
        const std::string sk =
            "q3.md3." + prefix + "_surf" + std::to_string(s);
        auto* vb = context.Get<SDL_GPUBuffer*>(
            sk + "_f" + std::to_string(frame) + "_vb", nullptr);
        auto* ib = context.Get<SDL_GPUBuffer*>(sk + "_ib", nullptr);
        const int numIdx = context.Get<int>(sk + "_num_idx", 0);
        if (!vb || !ib || numIdx <= 0) continue;
        auto* tex  = context.Get<SDL_GPUTexture*>(sk + "_tex", nullptr);
        auto* samp = context.Get<SDL_GPUSampler*>(sk + "_samp", nullptr);
        if (!tex || !samp) continue;

        auto* stex  = shadowTex ? shadowTex : tex;
        auto* ssamp = shadowSamp ? shadowSamp : samp;
        SDL_GPUTextureSamplerBinding b[2] = {{tex, samp}, {stex, ssamp}};
        SDL_BindGPUFragmentSamplers(pass, 0, b, 2);

        SDL_GPUBufferBinding vbBind = {vb, 0};
        SDL_BindGPUVertexBuffers(pass, 0, &vbBind, 1);
        SDL_GPUBufferBinding ibBind = {ib, 0};
        SDL_BindGPUIndexBuffer(pass, &ibBind,
                               SDL_GPU_INDEXELEMENTSIZE_16BIT);
        SDL_PushGPUVertexUniformData(cmd, 0, &vu, sizeof(vu));
        SDL_PushGPUFragmentUniformData(cmd, 0, &fu, sizeof(fu));
        SDL_DrawGPUIndexedPrimitives(pass, static_cast<uint32_t>(numIdx),
                                     1, 0, 0, 0);
    }
}

}  // namespace sdl3cpp::services::impl
