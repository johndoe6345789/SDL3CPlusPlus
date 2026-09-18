#pragma once

#include "services/interfaces/workflow/fs2024/terrain/fs2024_frustum.hpp"
#include "services/interfaces/workflow/fs2024/terrain/fs2024_terrain_uniforms.hpp"
#include "services/interfaces/workflow/fs2024/tiles/fs2024_tile_stream_state.hpp"

#include <SDL3/SDL_gpu.h>

namespace sdl3cpp::services::impl {

/// Draws the landmarks standing in one tile through the terrain
/// pipeline (bound by the caller): each instance's shared model pushed
/// with its own placement as the model matrix, every group with its own
/// texture. Groups without one use `untextured` (the generated walls'
/// kit). Instances the frustum cannot see are skipped whole.
void DrawFs2024TileLandmarks(
    SDL_GPURenderPass* pass, SDL_GPUCommandBuffer* cmd,
    const Fs2024LoadedTile& tile,
    const std::unordered_map<std::string, Fs2024LandmarkKitGpu>& kits,
    Fs2024TerrainVertexUniforms vertex,
    const Fs2024TerrainFragmentUniforms& fragment,
    const Fs2024Frustum& frustum, SDL_GPUTextureSamplerBinding untextured);

}  // namespace sdl3cpp::services::impl
