#include "services/interfaces/workflow/fs2024/tiles/fs2024_tile_lookup.hpp"

namespace sdl3cpp::services::impl {

const Fs2024Heightfield* Fs2024FindTileField(
    const Fs2024TileStreamState& state, float x, float z) {
    const auto it = state.resident.find(
        Fs2024TileKeyFor(x, z, state.tileSize));
    if (it == state.resident.end() || !it->second.terrain.loaded) {
        return nullptr;
    }
    return &it->second.terrain.field;
}

}  // namespace sdl3cpp::services::impl
