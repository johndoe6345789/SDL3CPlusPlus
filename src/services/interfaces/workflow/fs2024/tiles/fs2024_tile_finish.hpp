#pragma once

#include "services/interfaces/workflow/fs2024/landmark/fs2024_tile_landmarks.hpp"
#include "services/interfaces/workflow/fs2024/tiles/fs2024_prepared_tile.hpp"
#include "services/interfaces/workflow/fs2024/tiles/fs2024_tile_stream_state.hpp"

#include <SDL3/SDL_gpu.h>

class btDiscreteDynamicsWorld;

namespace sdl3cpp::services::impl {

/// Uploads a tile a loader thread built (main thread only: GPU uploads
/// and Bullet): its ground, land-class map, buildings and collision, at
/// its own corner of engine space.
Fs2024LoadedTile FinishFs2024Tile(SDL_GPUDevice* device,
                                  btDiscreteDynamicsWorld* physics,
                                  Fs2024World& world,
                                  Fs2024PreparedTile& prepared,
                                  Fs2024LandmarkKits& kits);

/// Puts on the GPU every landmark model the tile stands that `kits` does
/// not hold: the ones its loader decoded, and -- should a kit have been
/// released between the tile being built and finished -- any other,
/// decoded here.
void AdoptFs2024TileKits(SDL_GPUDevice* device, Fs2024World& world,
                         const Fs2024PreparedTile& prepared,
                         Fs2024LandmarkKits& kits);

}  // namespace sdl3cpp::services::impl
