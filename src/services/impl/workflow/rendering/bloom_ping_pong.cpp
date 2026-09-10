#include "services/interfaces/workflow/rendering/bloom_ping_pong.hpp"

namespace sdl3cpp::services::impl {

bool DrawBloomExtractPass(SDL_GPUCommandBuffer* cmd,
                          SDL_GPUGraphicsPipeline* pipeline,
                          SDL_GPUTexture* hdrTex, SDL_GPUSampler* sampler,
                          SDL_GPUTexture* target) {
    SDL_GPUColorTargetInfo colorTarget = {};
    colorTarget.texture                = target;
    colorTarget.load_op                = SDL_GPU_LOADOP_DONT_CARE;
    colorTarget.store_op               = SDL_GPU_STOREOP_STORE;

    SDL_GPURenderPass* pass =
        SDL_BeginGPURenderPass(cmd, &colorTarget, 1, nullptr);
    if (!pass) return false;

    SDL_BindGPUGraphicsPipeline(pass, pipeline);

    SDL_GPUTextureSamplerBinding hdrBinding = {};
    hdrBinding.texture                      = hdrTex;
    hdrBinding.sampler                      = sampler;
    SDL_BindGPUFragmentSamplers(pass, 0, &hdrBinding, 1);

    // Push bloom params: threshold=1.0, soft_knee=0.5
    struct {
        float params[4];
    } uniforms;
    uniforms.params[0] = 1.0f;  // threshold — luminance above this
                                // triggers bloom
    uniforms.params[1] = 0.5f;  // soft knee — smooth transition width
    uniforms.params[2] = 0.0f;
    uniforms.params[3] = 0.0f;
    SDL_PushGPUFragmentUniformData(cmd, 0, &uniforms, sizeof(uniforms));

    SDL_DrawGPUPrimitives(pass, 3, 1, 0, 0);
    SDL_EndGPURenderPass(pass);
    return true;
}

}  // namespace sdl3cpp::services::impl
