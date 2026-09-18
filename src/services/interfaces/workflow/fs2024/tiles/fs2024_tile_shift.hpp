#pragma once

#include "services/interfaces/workflow/fs2024/tiles/fs2024_tile_stream_state.hpp"
#include "services/interfaces/workflow/fs2024/world/fs2024_world_rebase.hpp"

class btDiscreteDynamicsWorld;

namespace sdl3cpp::services::impl {

/// Carries every resident tile across a scale-keeping re-base: its key
/// renumbered by the whole tiles the origin moved, its offset, ground
/// lookup and collision body moved by `rebase.shift`. Its meshes are in
/// its own space and are not touched, so nothing reloads and nothing
/// blinks. Work on the loader threads was cut for the old keys and is
/// thrown away; resolve wants afresh next frame.
void ShiftFs2024Tiles(Fs2024TileStreamState& state,
                      btDiscreteDynamicsWorld* physics,
                      const Fs2024Rebase& rebase);

}  // namespace sdl3cpp::services::impl
