#pragma once

#include "services/interfaces/workflow/fs2024/fs2024_tile_stream_state.hpp"

namespace sdl3cpp::services::impl {

/// Recomputes `state.pendingLoad`/`pendingEvict` for a player at
/// (x, z): every tile within `loadRadiusTiles` that is not already
/// resident or already pending goes on to load; every resident tile
/// beyond `evictRadiusTiles` goes to evict.
///
/// The gap between the two radii is deliberate, not slack: without it
/// a player standing on a tile boundary would load and evict the same
/// tile every frame. Pure and separately testable, since it is exactly
/// the part of "streaming" that is easy to get subtly wrong.
void Fs2024ResolveWantedTiles(Fs2024TileStreamState& state, float x,
                              float z);

}  // namespace sdl3cpp::services::impl
