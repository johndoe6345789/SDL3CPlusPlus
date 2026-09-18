#pragma once

#include "services/interfaces/workflow/fs2024/tiles/fs2024_tile_stream_state.hpp"

namespace sdl3cpp::services::impl {

/// Calls `draw` with every tile streaming chose to draw this frame.
template <typename Draw>
void ForEachDrawnFs2024Tile(const Fs2024TileStreamState& state, Draw&& draw) {
    for (const Fs2024TileKey& key : state.drawn) {
        const auto it = state.resident.find(key);
        if (it != state.resident.end()) draw(it->second);
    }
}

}  // namespace sdl3cpp::services::impl
