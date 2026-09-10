#include "services/interfaces/workflow/rendering/postfx_taa_resolve.hpp"

namespace sdl3cpp::services::impl {

void DrawTaaResolvePass(SDL_GPUCommandBuffer* cmd,
                        SDL_GPUGraphicsPipeline* pipeline,
                        SDL_GPUTexture* hdrTex,
                        const TaaHistoryTextures& history,
                        SDL_GPUSampler* sampler, float blendFactor,
                        uint32_t width, uint32_t height, double frameCount) {
    SDL_GPUColorTargetInfo colorTarget = {};
    colorTarget.texture                = history.write;
    colorTarget.load_op                = SDL_GPU_LOADOP_DONT_CARE;
    colorTarget.store_op               = SDL_GPU_STOREOP_STORE;

    SDL_GPURenderPass* pass =
        SDL_BeginGPURenderPass(cmd, &colorTarget, 1, nullptr);
    if (!pass) return;

    SDL_BindGPUGraphicsPipeline(pass, pipeline);

    SDL_GPUTextureSamplerBinding bindings[2] = {};
    bindings[0].texture                      = hdrTex;
    bindings[0].sampler                      = sampler;
    bindings[1].texture                      = history.read;
    bindings[1].sampler                      = sampler;
    SDL_BindGPUFragmentSamplers(pass, 0, bindings, 2);

    struct {
        float params[4];
    } uniforms;
    uniforms.params[0] = blendFactor;
    uniforms.params[1] = 1.0f / static_cast<float>(width);
    uniforms.params[2] = 1.0f / static_cast<float>(height);
    uniforms.params[3] = static_cast<float>(frameCount);
    SDL_PushGPUFragmentUniformData(cmd, 0, &uniforms, sizeof(uniforms));

    SDL_DrawGPUPrimitives(pass, 3, 1, 0, 0);
    SDL_EndGPURenderPass(pass);
}

}  // namespace sdl3cpp::services::impl
