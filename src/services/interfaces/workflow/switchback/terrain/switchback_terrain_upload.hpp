#pragma once

#include "services/interfaces/workflow/switchback/terrain/switchback_heightmap.hpp"
#include "services/interfaces/workflow/switchback/terrain/switchback_terrain_mesh.hpp"
#include "services/interfaces/workflow/switchback/terrain/switchback_terrain_state.hpp"
#include "services/interfaces/workflow/switchback/track/switchback_track_generate.hpp"

#include <SDL3/SDL_gpu.h>

#include <string>

namespace sdl3cpp::services::impl {

/// Uploads the layout's draw chunks and adds its collision heightfield.
/// Returns an empty string on success, otherwise the reason it failed.
std::string InstallSwitchbackTerrain(SDL_GPUDevice* device,
                                     btDiscreteDynamicsWorld& world,
                                     const SwitchbackTrackLayout& layout,
                                     int cells, float uvMetres,
                                     SwitchbackTerrainState& state);

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
