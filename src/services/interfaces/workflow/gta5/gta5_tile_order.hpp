#pragma once

#include "services/interfaces/workflow/gta5/gta5_stream_state.hpp"

#include <vector>

namespace sdl3cpp::services::impl {

/// The wanted tiles, nearest first.
///
/// The per-frame spawn budget is spent in this order so it goes on the
/// tile the player is standing in, rather than on whichever tile the hash
/// map happened to yield first.
std::vector<Gta5TileCoord> OrderGta5TilesByDistance(
    const Gta5StreamState& state);

}  // namespace sdl3cpp::services::impl
