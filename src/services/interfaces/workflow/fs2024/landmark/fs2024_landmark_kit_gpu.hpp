#pragma once

#include <SDL3/SDL_gpu.h>
#include <glm/glm.hpp>

#include <cstdint>
#include <vector>

namespace sdl3cpp::services::impl {

/// One landmark draw call's worth of uploaded geometry and its own
/// texture (a big landmark like a real building has more than one
/// material, unlike a tile's ground or its generated buildings).
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
    glm::vec3 min{0.f}, max{0.f};  ///< bounds of every group's vertices
};

}  // namespace sdl3cpp::services::impl
