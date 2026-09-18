#include "services/interfaces/workflow/fs2024/tiles/fs2024_tile_lookup.hpp"

namespace sdl3cpp::services::impl {

const Fs2024Heightfield* Fs2024FindTileField(
    const Fs2024TileStreamState& state, float x, float z) {
    // The finest resident tile there: it follows the ground closest.
    for (int level = kFs2024FinestLevel; level >= kFs2024CoarsestLevel;
         --level) {
        const auto it =
            state.resident.find(Fs2024TileKeyFor(x, z, state.tileSize, level));
        if (it != state.resident.end() && it->second.terrain.loaded) {
            return &it->second.terrain.field;
        }
    }
    return nullptr;
}

}  // namespace sdl3cpp::services::impl
