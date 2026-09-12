#pragma once

#include "services/interfaces/i_logger.hpp"
#include "services/interfaces/workflow/gta5/stream/gta5_stream_state.hpp"

#include <SDL3/SDL_gpu.h>

#include <memory>
#include <string>

namespace sdl3cpp::services::impl {

/// Spawn up to `budget` not-yet-spawned placements of one tile, appending
/// instances to the tile. Returns how many placements were consumed.
///
/// A placement is consumed whether or not it produced an instance: a
/// missing or oversized model must not be retried every frame.
int SpawnGta5TilePlacements(Gta5StreamState& state,
                            Gta5ResidentTile& resident, int budget,
                            SDL_GPUDevice* device,
                            btDiscreteDynamicsWorld* world,
                            const std::shared_ptr<ILogger>& logger);

}  // namespace sdl3cpp::services::impl
