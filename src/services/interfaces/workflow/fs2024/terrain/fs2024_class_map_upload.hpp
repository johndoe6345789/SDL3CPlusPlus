#pragma once

#include <SDL3/SDL_gpu.h>

#include <cstdint>
#include <vector>

namespace sdl3cpp::services::impl {

/// A tile's land-class map on the GPU: one R8 texel per class sample,
/// read with texelFetch -- never filtered, since blending two class
/// numbers would invent a third class that is not there.
struct Fs2024ClassMapGpu {
    SDL_GPUTexture* texture = nullptr;
    SDL_GPUSampler* sampler = nullptr;
};

/// Uploads `classes` (`size` x `size`, row 0 north) as R8_UNORM with a
/// nearest, clamped sampler. Throws std::runtime_error on failure,
/// releasing anything already created.
Fs2024ClassMapGpu UploadFs2024ClassMap(SDL_GPUDevice* device,
                                       const std::vector<std::uint8_t>& classes,
                                       int size);

}  // namespace sdl3cpp::services::impl
