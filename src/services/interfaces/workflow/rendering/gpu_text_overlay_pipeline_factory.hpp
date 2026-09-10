#pragma once

#include <SDL3/SDL_gpu.h>

namespace sdl3cpp::services::impl {

/// Builds the alpha-blended, depth-less pipeline the overlay quad draws
/// with. Vertex layout is float3 position + float2 uv.
SDL_GPUGraphicsPipeline* CreateOverlayPipeline(SDL_GPUDevice* device,
                                               SDL_GPUTextureFormat format,
                                               SDL_GPUShader* vertex,
                                               SDL_GPUShader* fragment);

}  // namespace sdl3cpp::services::impl
