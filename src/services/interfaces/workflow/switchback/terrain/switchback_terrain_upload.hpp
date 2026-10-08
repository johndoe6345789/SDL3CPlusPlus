#pragma once

#include "services/interfaces/workflow/switchback/terrain/switchback_heightmap.hpp"
#include "services/interfaces/workflow/switchback/terrain/switchback_terrain_mesh.hpp"
#include "services/interfaces/workflow/switchback/terrain/switchback_terrain_state.hpp"

#include <SDL3/SDL_gpu.h>

namespace sdl3cpp::services::impl {

/// Builds and uploads every chunk of the square heightmap. `base` supplies
/// the cell count, step and UV scale; each chunk's origin is filled in here.
/// Throws std::runtime_error when the GPU refuses a buffer.
void UploadSwitchbackChunks(SDL_GPUDevice* device,
                            const SwitchbackHeightmap& map,
                            const SwitchbackChunkSpec& base,
                            SwitchbackTerrainState& state);

void ReleaseSwitchbackChunks(SDL_GPUDevice* device,
                             SwitchbackTerrainState& state);

}  // namespace sdl3cpp::services::impl
