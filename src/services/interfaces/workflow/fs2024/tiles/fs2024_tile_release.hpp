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

/// Stops the loader threads' work: what is queued is dropped, what is
/// running finishes and is thrown away.
void StopFs2024Loads(Fs2024TileStreamState& state);

/// Stops the loader threads' work and releases every resident tile:
/// what shutdown needs, and a re-base that changes the ground scale,
/// since a tile built at the old scale would stand wrong.
void ReleaseAllFs2024Tiles(SDL_GPUDevice* device,
                           btDiscreteDynamicsWorld* world,
                           Fs2024TileStreamState& state);

}  // namespace sdl3cpp::services::impl
