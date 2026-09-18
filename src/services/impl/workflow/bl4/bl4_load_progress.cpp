#include "services/interfaces/workflow/bl4/bl4_load_progress.hpp"

#include "services/interfaces/workflow/bl4/bl4_tile_key.hpp"

namespace sdl3cpp::services::impl {

Bl4LoadProgress MeasureBl4LoadProgress(const Bl4TileStreamState& state,
                                       const glm::vec3& at) {
    const Bl4TileKey centre = Bl4TileKeyFor(at.x, at.z, state.tileSize);
    const int radius = state.loadRadiusTiles;
    int wanted = 0, ready = 0;
    for (int dz = -radius; dz <= radius; ++dz) {
        for (int dx = -radius; dx <= radius; ++dx) {
            const Bl4TileKey key{centre.x + dx, centre.z + dz};
            ++wanted;
            // A tile with no placements.json is outside the baked
            // region: nothing will ever arrive, so it counts as in.
            if (state.resident.count(key) != 0 || state.missing.count(key) != 0) {
                ++ready;
            }
        }
    }
    Bl4LoadProgress progress;
    progress.done = wanted == 0 || ready == wanted;
    if (!progress.done) {
        const int percent = wanted > 0 ? (ready * 100) / wanted : 100;
        progress.text = "LOADING " + std::to_string(percent) + "%";
    }
    return progress;
}

}  // namespace sdl3cpp::services::impl
