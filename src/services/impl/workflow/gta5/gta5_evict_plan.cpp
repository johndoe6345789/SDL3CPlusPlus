#include "services/interfaces/workflow/gta5/gta5_evict_plan.hpp"

#include "services/interfaces/workflow/gta5/gta5_collision_body.hpp"
#include "services/interfaces/workflow/gta5/gta5_grid.hpp"

#include <vector>

namespace sdl3cpp::services::impl {
namespace {

/// Give up this tile's hold on every archetype it placed.
int ReleaseInstances(Gta5ResidentTile& resident,
                     btDiscreteDynamicsWorld* world) {
    for (Gta5Instance& instance : resident.instances) {
        RemoveGta5InstanceBody(world, instance);
        if (instance.geometry && instance.geometry->references > 0) {
            --instance.geometry->references;
        }
    }
    const auto released = static_cast<int>(resident.instances.size());
    resident.instances.clear();
    return released;
}

}  // namespace

Gta5EvictResult ApplyGta5EvictPlan(Gta5StreamState& state,
                                  btDiscreteDynamicsWorld* world) {
    Gta5EvictResult result;

    const float tileSize =
        state.world.tileSize > 0.f ? state.world.tileSize : 512.f;
    const float evictDistance =
        (static_cast<float>(state.streaming.evictRadiusTiles) + 0.5f) *
        tileSize;

    std::vector<Gta5TileCoord> dropped;
    for (auto& entry : state.resident) {
        Gta5ResidentTile& resident = entry.second;
        // Horizontal only: tiles are columns, and resolve picks them by x
        // and z. Measured in 3D, a player who fell below the map evicted
        // the tile under them every frame while resolve asked for it back.
        const glm::vec3 centre = Gta5TileCentre(state.world, entry.first);
        const float distance =
            glm::distance(glm::vec2(centre.x, centre.z),
                          glm::vec2(state.centreOrigin.x,
                                    state.centreOrigin.z));

        if (distance > evictDistance) {
            result.instancesReleased += ReleaseInstances(resident, world);
            dropped.push_back(entry.first);
            continue;
        }

        if (state.rebuild.find(entry.first) != state.rebuild.end()) {
            // Keep the placements: they came off disk and have not
            // changed. Only the instances go, and the band is recomputed
            // so load rebuilds at the new detail level.
            result.instancesReleased += ReleaseInstances(resident, world);
            resident.spawnedCount = 0;
            resident.bandAtSpawn =
                Gta5BandForDistance(state.world, distance);
            ++result.rebuilt;
        }
    }

    state.rebuild.clear();
    for (const Gta5TileCoord& tile : dropped) {
        state.resident.erase(tile);
    }
    result.dropped = static_cast<int>(dropped.size());
    return result;
}

}  // namespace sdl3cpp::services::impl
