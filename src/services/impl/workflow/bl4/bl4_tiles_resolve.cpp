#include "services/interfaces/workflow/bl4/bl4_tiles_resolve.hpp"

#include <algorithm>

namespace sdl3cpp::services::impl {

void Bl4ResolveWantedTiles(Bl4TileStreamState& state, float x, float z) {
    const Bl4TileKey centre = Bl4TileKeyFor(x, z, state.tileSize);

    state.pendingEvict.clear();
    for (const auto& [key, tile] : state.resident) {
        if (Bl4TileRing(key, centre) > state.evictRadiusTiles) {
            state.pendingEvict.push_back(key);
        }
    }

    const auto alreadyWanted = [&](const Bl4TileKey& key) {
        return state.resident.count(key) != 0 || state.missing.count(key) != 0 ||
              std::find(state.pendingLoad.begin(), state.pendingLoad.end(), key) !=
                  state.pendingLoad.end();
    };
    for (int dz = -state.loadRadiusTiles; dz <= state.loadRadiusTiles; ++dz) {
        for (int dx = -state.loadRadiusTiles; dx <= state.loadRadiusTiles; ++dx) {
            const Bl4TileKey key{centre.x + dx, centre.z + dz};
            if (!alreadyWanted(key)) state.pendingLoad.push_back(key);
        }
    }
    // Nearest first, so a sudden jump (spawn, teleport) fills the
    // ground directly underfoot before the tiles at the radius's edge.
    std::stable_sort(state.pendingLoad.begin(), state.pendingLoad.end(),
                     [&](const Bl4TileKey& a, const Bl4TileKey& b) {
                         return Bl4TileRing(a, centre) < Bl4TileRing(b, centre);
                     });
}

}  // namespace sdl3cpp::services::impl
