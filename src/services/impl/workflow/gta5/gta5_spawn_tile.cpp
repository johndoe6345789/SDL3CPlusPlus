#include "services/interfaces/workflow/gta5/gta5_spawn_tile.hpp"

#include "services/interfaces/workflow/gta5/gta5_geometry_cache.hpp"
#include "services/interfaces/workflow/gta5/gta5_model_matrix.hpp"

#include <utility>

namespace sdl3cpp::services::impl {
namespace {

SceneObject MakeObject(const Gta5Placement& placement,
                       const Gta5Geometry& geometry,
                       const Gta5SpawnOptions& options,
                       const std::string& tag) {
    SceneObject object;
    object.objectType = options.objectTypePrefix + tag;
    object.vertices = geometry.vertices;
    object.indices = geometry.indices;
    object.shaderKeys = {options.shaderKey};
    object.modelMatrix = BuildGta5ModelMatrix(placement);
    object.hasCustomModelMatrix = true;
    object.computeModelMatrixRef = -1;
    return object;
}

}  // namespace

int SpawnGta5TilePlacements(Gta5StreamState& state,
                            const Gta5TileCoord& tile,
                            Gta5ResidentTile& resident,
                            const Gta5SpawnOptions& options, int budget,
                            std::vector<SceneObject>& objects,
                            const std::shared_ptr<ILogger>& logger) {
    const std::string tag = MakeGta5TileTag(tile);
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

        const Gta5Geometry* geometry =
            GetOrLoadGta5Geometry(state, placement, logger);
        if (!geometry) continue;

        objects.push_back(MakeObject(placement, *geometry, options, tag));
    }

    return consumed;
}

}  // namespace sdl3cpp::services::impl
