#pragma once

#include "services/interfaces/workflow/fs2024/landmark/fs2024_landmark_mesh.hpp"

#include <SDL3/SDL_gpu.h>

#include <cstdint>
#include <vector>

namespace sdl3cpp::services::impl {

/// One landmark draw call's worth of uploaded geometry and the colour
/// map it samples (owned by its kit, which may share one map between
/// many groups; null when it has none).
struct Fs2024LandmarkGroupGpu {
    SDL_GPUTexture* texture = nullptr;
    SDL_GPUSampler* sampler = nullptr;
    SDL_GPUBuffer* vertexBuffer = nullptr;
    SDL_GPUBuffer* indexBuffer = nullptr;
    std::uint32_t indexCount = 0;
};

/// A landmark model's whole uploaded kit, in its own space -- kept once
/// however many instances of it stand in the world.
struct Fs2024LandmarkKitGpu {
    std::vector<Fs2024LandmarkGroupGpu> groups;
    std::vector<SDL_GPUTexture*> textures;  ///< owned
    std::vector<SDL_GPUSampler*> samplers;  ///< owned
    Fs2024LandmarkBounds bounds;
};

}  // namespace sdl3cpp::services::impl
