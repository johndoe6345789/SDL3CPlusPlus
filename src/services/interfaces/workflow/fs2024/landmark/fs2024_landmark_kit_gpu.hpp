#pragma once

#include <SDL3/SDL_gpu.h>

#include <cstdint>
#include <vector>

namespace sdl3cpp::services::impl {

/// One landmark draw call's worth of uploaded geometry and its own
/// texture (a big landmark like a real building has more than one
/// material, unlike a tile's ground or its OSM box massing).
struct Fs2024LandmarkGroupGpu {
    SDL_GPUTexture* texture = nullptr;
    SDL_GPUSampler* sampler = nullptr;
    SDL_GPUBuffer* vertexBuffer = nullptr;
    SDL_GPUBuffer* indexBuffer = nullptr;
    std::uint32_t indexCount = 0;
};

/// A landmark model's whole uploaded kit -- shared, global data, kept
/// once regardless of how many tiles/instances reference it by name.
struct Fs2024LandmarkKitGpu {
    std::vector<Fs2024LandmarkGroupGpu> groups;
};

}  // namespace sdl3cpp::services::impl
