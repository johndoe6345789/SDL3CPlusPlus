#pragma once

#include "services/interfaces/workflow/switchback/terrain/switchback_heightmap.hpp"

#include <SDL3/SDL_gpu.h>

#include <glm/glm.hpp>

#include <cstdint>
#include <vector>

class btDiscreteDynamicsWorld;
class btHeightfieldTerrainShape;
class btRigidBody;

namespace sdl3cpp::services::impl {

/// One square of terrain on the GPU, drawn in a single call.
struct SwitchbackTerrainChunk {
    SDL_GPUBuffer* vertexBuffer = nullptr;
    SDL_GPUBuffer* indexBuffer = nullptr;
    std::uint32_t indexCount = 0;
};

/// The ground the Switchback game drives on: draw chunks and the static
/// Bullet heightfield under them. Lives for the whole run.
struct SwitchbackTerrainState {
    bool attempted = false;
    std::vector<SwitchbackTerrainChunk> chunks;
    std::vector<float> collisionHeights;
    btHeightfieldTerrainShape* shape = nullptr;
    btRigidBody* body = nullptr;
    /// The race gates of the loaded track, start line first.
    std::vector<glm::vec3> checkpoints;
};

/// Adds the static heightfield body to `world`, with samples `stepM` apart
/// and the grid centred on the origin.
void BuildSwitchbackCollision(btDiscreteDynamicsWorld& world,
                              const SwitchbackHeightmap& map, float stepM,
                              float heightMaxM, SwitchbackTerrainState& state);

}  // namespace sdl3cpp::services::impl
