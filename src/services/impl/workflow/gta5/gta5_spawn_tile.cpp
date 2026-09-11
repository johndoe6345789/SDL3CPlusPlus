#include "services/interfaces/workflow/gta5/gta5_spawn_tile.hpp"

#include "services/interfaces/workflow/gta5/gta5_asset_index.hpp"
#include "services/interfaces/workflow/gta5/gta5_collision_body.hpp"
#include "services/interfaces/workflow/gta5/gta5_geometry_cache.hpp"
#include "services/interfaces/workflow/gta5/gta5_model_matrix.hpp"
#include "services/interfaces/workflow/gta5/gta5_proxy.hpp"

namespace sdl3cpp::services::impl {
namespace {

/// A stand-in for a special pass -- see Gta5ProxyKind -- which the view
/// never draws. Land, LOD and prop "proxies" are scenery and still are.
bool IsPassProxy(const Gta5StreamState& state, std::uint32_t hash) {
    return state.assets && hash != 0 &&
           ClassifyGta5Proxy(Gta5ArchetypeName(*state.assets, hash)) !=
               Gta5ProxyKind::None;
}

}  // namespace

int SpawnGta5TilePlacements(Gta5StreamState& state,
                            Gta5ResidentTile& resident, int budget,
                            SDL_GPUDevice* device,
                            btDiscreteDynamicsWorld* world,
                            const std::shared_ptr<ILogger>& logger) {
    int consumed = 0;

    while (resident.spawnedCount < resident.placements.size() &&
           consumed < budget) {
        const Gta5Placement& placement =
            resident.placements[resident.spawnedCount];

        // Placements authored for a finer band than the tile is drawn at
        // are skipped: at SLOD range we want the merged shells, not every
        // railing. Nor are pass proxies drawn at all.
        if (static_cast<int>(placement.lod) <
                static_cast<int>(resident.bandAtSpawn) ||
            IsPassProxy(state, placement.archetypeHash)) {
            ++resident.spawnedCount;
            ++consumed;
            continue;
        }

        bool pending = false;
        Gta5Geometry* geometry =
            GetOrLoadGta5Geometry(state, placement, device, logger, &pending);
        // Still being prepared off-thread: this tile resumes here once it
        // lands, and the budget goes to tiles that are ready.
        if (pending) break;
        ++resident.spawnedCount;
        ++consumed;
        if (!geometry) continue;

        Gta5Instance instance;
        instance.geometry = geometry;
        instance.modelMatrix = BuildGta5ModelMatrix(placement);
        instance.lodDist = placement.lodDist;
        instance.archetype = placement.archetypeHash;
        // Always handed over up close. Waiving it for a tile's finest band
        // kept every parent whose lodDist fell in the HD band -- and many
        // do -- drawn over its own children: blurry lumps on the road.
        instance.childLodDist = placement.childLodDist;
        // Only the finest level collides: a parent's coarse mesh, hidden
        // up close, still stood in the road as an invisible lump.
        if (placement.childLodDist <= 0.f) {
            AddGta5InstanceBody(world, placement, *geometry, instance);
        }
        resident.instances.push_back(instance);
        ++geometry->references;
    }

    return consumed;
}

}  // namespace sdl3cpp::services::impl
