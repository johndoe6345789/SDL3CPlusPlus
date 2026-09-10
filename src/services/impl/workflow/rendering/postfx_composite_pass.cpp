#include "services/interfaces/workflow/rendering/postfx_composite_pass.hpp"

namespace sdl3cpp::services::impl {

void DrawPostfxCompositeQuad(SDL_GPURenderPass* pass,
                             SDL_GPUGraphicsPipeline* pipeline,
                             SDL_GPUTexture* hdr, SDL_GPUSampler* sampler,
                             SDL_GPUTexture* ssao, SDL_GPUTexture* bloom) {
    SDL_BindGPUGraphicsPipeline(pass, pipeline);

    SDL_GPUTextureSamplerBinding bindings[3] = {};
    bindings[0].texture                      = hdr;
    bindings[0].sampler                      = sampler;
    bindings[1].texture                      = ssao ? ssao : hdr;
    bindings[1].sampler                      = sampler;
    bindings[2].texture                      = bloom ? bloom : hdr;
    bindings[2].sampler                      = sampler;
    SDL_BindGPUFragmentSamplers(pass, 0, bindings, 3);

    SDL_DrawGPUPrimitives(pass, 3, 1, 0, 0);
}

}  // namespace sdl3cpp::services::impl
