#include "services/interfaces/workflow/gta5/gta5_load_progress.hpp"

#include "services/interfaces/workflow/gta5/gta5_grid.hpp"

namespace sdl3cpp::services::impl {

Gta5LoadProgress MeasureGta5LoadProgress(const Gta5StreamState& state,
                                         const glm::vec3& at) {
    Gta5LoadProgress progress;
    if (state.assetsPending.valid()) {
        progress.text = "Indexing map...";
        return progress;
    }
    const auto found =
        state.resident.find(Gta5TileForPosition(state.world, at));
    if (found == state.resident.end() || !found->second.placementsRead) {
        progress.text = "Reading map...";
        return progress;
    }
    const Gta5ResidentTile& tile = found->second;
    const std::size_t total = tile.placements.size();
    if (tile.spawnedCount >= total) {
        progress.done = true;  // out at sea, total is 0: nothing to wait for
        return progress;
    }
    const auto percent = static_cast<int>(100 * tile.spawnedCount / total);
    progress.text = "Loading: " + std::to_string(percent) + "%";
    return progress;
}

}  // namespace sdl3cpp::services::impl
