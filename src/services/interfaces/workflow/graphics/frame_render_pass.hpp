#pragma once

#include <SDL3/SDL_gpu.h>

namespace sdl3cpp::services::impl {

/**
 * @brief Begins the frame's render pass, clearing `colorTarget` to
 * (r,g,b,a) and `depthTarget` to 1.0.
 * @throws std::runtime_error (naming SDL_GetError()) on failure.
 */
SDL_GPURenderPass* BeginFrameRenderPassOrThrow(SDL_GPUCommandBuffer* cmd,
                                               SDL_GPUTexture* colorTarget,
                                               SDL_GPUTexture* depthTarget,
                                               float r, float g, float b,
                                               float a);

}  // namespace sdl3cpp::services::impl
