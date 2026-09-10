#include "services/interfaces/workflow/gta5/gta5_evict_plan.hpp"

#include "services/interfaces/workflow/gta5/gta5_grid.hpp"

namespace sdl3cpp::services::impl {

Gta5EvictPlan ApplyGta5EvictPlan(Gta5StreamState& state,
                                 const std::string& objectTypePrefix) {
    Gta5EvictPlan plan;

    const float tileSize =
        state.world.tileSize > 0.f ? state.world.tileSize : 512.f;
    const float evictDistance =
        (static_cast<float>(state.streaming.evictRadiusTiles) + 0.5f) *
        tileSize;

    for (auto& entry : state.resident) {
        const Gta5TileCoord& tile = entry.first;
        Gta5ResidentTile& resident = entry.second;

        const float distance = glm::distance(
            Gta5TileCentre(state.world, tile), state.centreOrigin);
        const bool needsRebuild =
            state.rebuild.find(tile) != state.rebuild.end();

        if (distance > evictDistance) {
            plan.purge.insert(objectTypePrefix + MakeGta5TileTag(tile));
            plan.dropped.push_back(tile);
            continue;
        }

        if (needsRebuild) {
            // Keep the placements: they came off disk and have not
            // changed. Only the spawned objects go, and the band is
            // recomputed so load rebuilds at the new detail level.
            plan.purge.insert(objectTypePrefix + MakeGta5TileTag(tile));
            resident.spawnedCount = 0;
            resident.bandAtSpawn =
                Gta5BandForDistance(state.world, distance);
            ++plan.rebuilt;
        }
    }

    state.rebuild.clear();
    for (const Gta5TileCoord& tile : plan.dropped) {
        state.resident.erase(tile);
    }
    return plan;
}

}  // namespace sdl3cpp::services::impl
