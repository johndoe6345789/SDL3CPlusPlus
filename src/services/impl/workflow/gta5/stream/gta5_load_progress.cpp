#include "services/interfaces/workflow/gta5/stream/gta5_load_progress.hpp"

#include "services/interfaces/workflow/gta5/stream/gta5_grid.hpp"

#include <algorithm>

namespace sdl3cpp::services::impl {

Gta5LoadProgress MeasureGta5LoadProgress(const Gta5StreamState& state,
                                         const glm::vec3& at) {
    Gta5LoadProgress progress;
    if (state.assetsPending.valid() || state.wanted.empty()) {
        progress.text = "Indexing map...";
        return progress;
    }
    // The tile under the point and its eight neighbours. Entities are
    // tiled by origin, and a terrain piece is big: its origin can sit in
    // the next tile while its surface covers this one. Waiting for the
    // centre alone let a car drop into the desert before the ground under
    // it had arrived. Only tiles the streamer wants count, so the sea and
    // the map's edge cannot hold anything up.
    const Gta5TileCoord centre = Gta5TileForPosition(state.world, at);
    std::size_t spawned = 0;
    std::size_t total = 0;
    for (int dz = -1; dz <= 1; ++dz) {
        for (int dx = -1; dx <= 1; ++dx) {
            const Gta5TileCoord tile{centre.x + dx, centre.z + dz};
            if (state.wanted.find(tile) == state.wanted.end()) continue;
            const auto found = state.resident.find(tile);
            if (found == state.resident.end() ||
                !found->second.placementsRead) {
                progress.text = "Reading map...";
                return progress;
            }
            spawned += std::min(found->second.spawnedCount,
                                found->second.placements.size());
            total += found->second.placements.size();
        }
    }
    if (spawned >= total) {
        progress.done = true;  // all nine in, or nothing to wait for
        return progress;
    }
    const auto percent = static_cast<int>(100 * spawned / total);
    progress.text = "Loading: " + std::to_string(percent) + "%";
    return progress;
}

}  // namespace sdl3cpp::services::impl
