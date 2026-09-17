#pragma once

#include "services/interfaces/workflow/fs2024/terrain/fs2024_terrain_state.hpp"

namespace sdl3cpp::services::impl {

/// Split `state.field` into blocks of `cells` cells and upload each.
/// Returns the number of triangles uploaded.
std::size_t UploadFs2024TerrainChunks(SDL_GPUDevice* device,
                                      Fs2024TerrainState& state,
                                      int cells);

/// Release every block's buffers and forget them.
void ReleaseFs2024TerrainChunks(SDL_GPUDevice* device,
                                Fs2024TerrainState& state);

}  // namespace sdl3cpp::services::impl
