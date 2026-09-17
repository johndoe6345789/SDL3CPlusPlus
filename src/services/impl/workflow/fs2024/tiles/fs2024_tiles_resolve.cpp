#include "services/interfaces/workflow/fs2024/tiles/fs2024_tiles_resolve.hpp"

#include <algorithm>

namespace sdl3cpp::services::impl {

void Fs2024ResolveWantedTiles(Fs2024TileStreamState& state, float x,
                              float z) {
    const Fs2024TileKey centre =
        Fs2024TileKeyFor(x, z, state.tileSize);

    state.pendingEvict.clear();
    for (const auto& [key, tile] : state.resident) {
        if (Fs2024TileRing(key, centre) > state.evictRadiusTiles) {
            state.pendingEvict.push_back(key);
        }
    }

    const auto alreadyWanted = [&](const Fs2024TileKey& key) {
        return state.resident.count(key) != 0 ||
              state.missing.count(key) != 0 ||
              std::find(state.pendingLoad.begin(), state.pendingLoad.end(),
                        key) != state.pendingLoad.end();
    };
    for (int dz = -state.loadRadiusTiles; dz <= state.loadRadiusTiles; ++dz) {
        for (int dx = -state.loadRadiusTiles; dx <= state.loadRadiusTiles;
             ++dx) {
            const Fs2024TileKey key{centre.x + dx, centre.z + dz};
            if (!alreadyWanted(key)) state.pendingLoad.push_back(key);
        }
    }
    // Nearest first, so a sudden jump (spawn, teleport) fills the
    // ground directly underfoot before the tiles at the radius's edge.
    std::stable_sort(state.pendingLoad.begin(), state.pendingLoad.end(),
                     [&](const Fs2024TileKey& a, const Fs2024TileKey& b) {
                         return Fs2024TileRing(a, centre) <
                               Fs2024TileRing(b, centre);
                     });
}

}  // namespace sdl3cpp::services::impl
