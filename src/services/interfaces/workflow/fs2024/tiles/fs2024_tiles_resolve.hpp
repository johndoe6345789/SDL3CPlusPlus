#pragma once

#include "services/interfaces/workflow/fs2024/tiles/fs2024_tile_stream_state.hpp"

namespace sdl3cpp::services::impl {

/// Recomputes what streaming wants for a viewer at `viewer` (engine x,
/// z; y its height above the ground): the LOD cut (SelectFs2024Tiles),
/// then which resident tiles to draw and evict and which to load
/// (PlanFs2024Tiles). Tiles already on the loader threads are not
/// queued again, and failures no longer wanted are forgotten so that
/// coming back retries them. Pure and separately testable, since it is
/// exactly the part of streaming that is easy to get subtly wrong.
void Fs2024ResolveWantedTiles(Fs2024TileStreamState& state,
                              const glm::vec3& viewer);

}  // namespace sdl3cpp::services::impl
