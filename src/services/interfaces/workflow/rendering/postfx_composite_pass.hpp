#pragma once

#include <SDL3/SDL_gpu.h>

namespace sdl3cpp::services::impl {

/**
 * @brief Draws the fullscreen composite triangle into an already-begun pass.
 *
 * Binds `hdr` to slot 0 and falls back to `hdr` for slots 1 (SSAO) and 2
 * (bloom) when those textures are null, so the shader always samples a valid
 * texture even with those effects disabled.
 */
void DrawPostfxCompositeQuad(SDL_GPURenderPass* pass,
                             SDL_GPUGraphicsPipeline* pipeline,
                             SDL_GPUTexture* hdr, SDL_GPUSampler* sampler,
                             SDL_GPUTexture* ssao, SDL_GPUTexture* bloom);

}  // namespace sdl3cpp::services::impl
