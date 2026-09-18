#pragma once

#include "services/interfaces/workflow/fs2024/tiles/fs2024_tile_stream_state.hpp"

#include <SDL3/SDL_gpu.h>

class btDiscreteDynamicsWorld;

namespace sdl3cpp::services::impl {

/// Releases everything one resident tile holds: its ground and building
/// buffers, its class map, and its collision body. Shared by eviction,
/// shutdown and an origin re-base, which drops every tile at once.
void ReleaseFs2024Tile(SDL_GPUDevice* device, btDiscreteDynamicsWorld* world,
                       Fs2024LoadedTile& tile);

}  // namespace sdl3cpp::services::impl
