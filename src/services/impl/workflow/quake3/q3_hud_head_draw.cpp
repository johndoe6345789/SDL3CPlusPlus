#include "services/interfaces/workflow/quake3/q3_hud_head_draw.hpp"

#include <glm/gtc/type_ptr.hpp>

#include <cstring>

namespace sdl3cpp::services::impl {

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
    vu.normal[1]   = 1.0f;
    vu.uv_scale[0] = 1.0f;
    vu.uv_scale[1] = 1.0f;

    // Identity shadow VP — no shadows in portrait
    const glm::mat4 noShadow(1.0f);
    std::memcpy(vu.shadow_vp, glm::value_ptr(noShadow), sizeof(float) * 16);

    for (int s = 0; s < nSurfs; ++s) {
        const std::string sk = "q3.md3." + prefix + "_surf" + std::to_string(s);
        auto* vb       = context.Get<SDL_GPUBuffer*>(sk + "_f0_vb", nullptr);
        auto* ib       = context.Get<SDL_GPUBuffer*>(sk + "_ib", nullptr);
        const int nIdx = context.Get<int>(sk + "_num_idx", 0);
        if (!vb || !ib || nIdx <= 0) continue;

        auto* tex  = context.Get<SDL_GPUTexture*>(sk + "_tex", nullptr);
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

}  // namespace sdl3cpp::services::impl
