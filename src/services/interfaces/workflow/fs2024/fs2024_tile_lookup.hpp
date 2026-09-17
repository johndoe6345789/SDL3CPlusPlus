#pragma once

#include "services/interfaces/workflow/fs2024/fs2024_tile_stream_state.hpp"

namespace sdl3cpp::services::impl {

/// The heightfield of whichever resident tile contains world (x, z),
/// or nullptr when that tile is not loaded (not streamed in yet, or
/// outside every baked area).
const Fs2024Heightfield* Fs2024FindTileField(
    const Fs2024TileStreamState& state, float x, float z);

}  // namespace sdl3cpp::services::impl
