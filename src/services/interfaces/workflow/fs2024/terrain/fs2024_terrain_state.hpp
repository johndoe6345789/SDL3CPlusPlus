#pragma once

#include "services/interfaces/workflow/fs2024/terrain/fs2024_heightfield.hpp"
#include "services/interfaces/workflow/fs2024/terrain/fs2024_terrain_collision.hpp"

#include <SDL3/SDL_gpu.h>

#include <cstdint>
#include <vector>

namespace sdl3cpp::services::impl {

/// One uploaded block of ground.
struct Fs2024TerrainChunkGpu {
    SDL_GPUBuffer* vertexBuffer = nullptr;
    SDL_GPUBuffer* indexBuffer = nullptr;
    std::uint32_t indexCount = 0;
    glm::vec3 min{0.f};
    glm::vec3 max{0.f};
};

/// Everything the fs2024.terrain.* steps share. Held by the registrar
/// rather than the context: the heightfield is millions of floats, and
/// the collision shape reads them in place.
struct Fs2024TerrainState {
    Fs2024Heightfield field;
    std::vector<Fs2024TerrainChunkGpu> chunks;
    Fs2024TerrainCollision collision;
    bool loaded = false;
};

}  // namespace sdl3cpp::services::impl
