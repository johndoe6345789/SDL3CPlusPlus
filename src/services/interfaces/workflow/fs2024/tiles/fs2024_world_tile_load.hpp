#pragma once

#include "services/interfaces/workflow/fs2024/landmark/fs2024_tile_landmarks.hpp"
#include "services/interfaces/workflow/fs2024/tiles/fs2024_tile_stream_state.hpp"

#include <SDL3/SDL_gpu.h>

class btDiscreteDynamicsWorld;

namespace sdl3cpp::services::impl {

/// Builds and uploads one streaming tile from FS2024's own data: its
/// ground from the game's DEM, its land-class map from its ground-cover
/// layer, its buildings from its footprint library, its landmarks
/// where FS2024 places them -- loading any model `kits` lacks, and
/// clearing the generated buildings under each. Main thread only (GPU
/// uploads and Bullet). A tile with no data at all -- open ocean --
/// still comes back as flat ground at sea level, never as a hole.
Fs2024LoadedTile LoadFs2024WorldTile(SDL_GPUDevice* device,
                                     btDiscreteDynamicsWorld* physics,
                                     Fs2024World& world,
                                     const Fs2024TileKey& key,
                                     Fs2024LandmarkKits& kits);

}  // namespace sdl3cpp::services::impl
