#pragma once

#include "services/interfaces/workflow/gta5/gta5_stream_state.hpp"

namespace sdl3cpp::services::impl {

/// Ask the load pool for a map archetype's geometry.
///
/// Returns it when ready. Otherwise nullptr, with `pending` set while it
/// is being prepared -- it is queued the first time it is asked for --
/// and clear once it has been tried and found to have nothing to draw.
Gta5Geometry* RequestGta5IndexedGeometry(Gta5StreamState& state,
                                         const Gta5Placement& placement,
                                         bool& pending);

/// Queue every archetype a tile will spawn, at the band it will spawn
/// at. Called when the tile is read and again when it is rebuilt, so a
/// district is prepared on every core at once rather than one archetype
/// after another as spawning reaches them.
void PrefetchGta5Tile(Gta5StreamState& state, Gta5ResidentTile& resident);

}  // namespace sdl3cpp::services::impl
