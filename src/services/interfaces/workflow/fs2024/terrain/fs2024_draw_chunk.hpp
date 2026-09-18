#pragma once

#include "services/interfaces/workflow/fs2024/terrain/fs2024_frustum.hpp"
#include "services/interfaces/workflow/fs2024/terrain/fs2024_terrain_state.hpp"

#include <SDL3/SDL_gpu.h>

namespace sdl3cpp::services::impl {

/// Draws one tile-local chunk with whatever pipeline, uniforms and
/// samplers are bound, unless the frustum cannot see it where its tile
/// puts it (`offset`).
void DrawFs2024Chunk(SDL_GPURenderPass* pass,
                     const Fs2024TerrainChunkGpu& chunk,
                     const Fs2024Frustum& frustum, const glm::vec3& offset);

}  // namespace sdl3cpp::services::impl
