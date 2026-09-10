#include "services/interfaces/workflow/rendering/postfx_bloom_blur_helpers.hpp"

namespace sdl3cpp::services::impl {

bool DrawBloomBlurPass(SDL_GPUCommandBuffer* cmd,
                       SDL_GPUGraphicsPipeline* pipeline,
                       SDL_GPUTexture* srcTex, SDL_GPUTexture* dstTex,
                       SDL_GPUSampler* sampler, float dirX, float dirY) {
    SDL_GPUColorTargetInfo colorTarget = {};
    colorTarget.texture                = dstTex;
    colorTarget.load_op                = SDL_GPU_LOADOP_DONT_CARE;
    colorTarget.store_op               = SDL_GPU_STOREOP_STORE;

    SDL_GPURenderPass* pass =
        SDL_BeginGPURenderPass(cmd, &colorTarget, 1, nullptr);
    if (!pass) return false;

    SDL_BindGPUGraphicsPipeline(pass, pipeline);

    SDL_GPUTextureSamplerBinding binding = {};
    binding.texture                      = srcTex;
    binding.sampler                      = sampler;
    SDL_BindGPUFragmentSamplers(pass, 0, &binding, 1);

    struct {
        float direction[4];
    } uniforms;
    uniforms.direction[0] = dirX;
    uniforms.direction[1] = dirY;
    uniforms.direction[2] = 0.0f;
    uniforms.direction[3] = 0.0f;
    SDL_PushGPUFragmentUniformData(cmd, 0, &uniforms, sizeof(uniforms));

    SDL_DrawGPUPrimitives(pass, 3, 1, 0, 0);
    SDL_EndGPURenderPass(pass);
    return true;
}

}  // namespace sdl3cpp::services::impl
