#pragma once

#include <SDL3/SDL.h>
#include <SDL3/SDL_gpu.h>

namespace sdl3cpp::services::impl {

/**
 * @brief Returns `existing` if non-null, otherwise creates a
 * D32_FLOAT depth texture sized `width`x`height`.
 * @throws std::runtime_error (naming SDL_GetError()) on create failure.
 */
SDL_GPUTexture* GetOrCreateFrameDepthTexture(SDL_GPUDevice* device,
                                             SDL_GPUTexture* existing,
                                             Uint32 width, Uint32 height);

}  // namespace sdl3cpp::services::impl
