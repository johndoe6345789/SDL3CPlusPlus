#include "services/interfaces/workflow/gta5/gta5_spawn_tile.hpp"

#include "services/interfaces/workflow/gta5/gta5_geometry_cache.hpp"
#include "services/interfaces/workflow/gta5/gta5_model_matrix.hpp"

namespace sdl3cpp::services::impl {

int SpawnGta5TilePlacements(Gta5StreamState& state,
                            Gta5ResidentTile& resident, int budget,
                            SDL_GPUDevice* device,
                            const std::shared_ptr<ILogger>& logger) {
    int consumed = 0;

    while (resident.spawnedCount < resident.placements.size() &&
           consumed < budget) {
        const Gta5Placement& placement =
            resident.placements[resident.spawnedCount];
        ++resident.spawnedCount;
        ++consumed;

        // Placements authored for a finer band than the tile is drawn at
        // are skipped: at SLOD range we want the merged shells, not every
        // railing.
        if (static_cast<int>(placement.lod) <
            static_cast<int>(resident.bandAtSpawn)) {
            continue;
        }

        Gta5Geometry* geometry =
            GetOrLoadGta5Geometry(state, placement, device, logger);
        if (!geometry) continue;

        Gta5Instance instance;
        instance.geometry = geometry;
        instance.modelMatrix = BuildGta5ModelMatrix(placement);
        resident.instances.push_back(instance);
        ++geometry->references;
    }

    return consumed;
}

}  // namespace sdl3cpp::services::impl
