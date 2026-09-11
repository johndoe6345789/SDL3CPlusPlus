#pragma once

#include "services/interfaces/workflow/gta5/gta5_asset_index.hpp"
#include "services/interfaces/workflow/gta5/gta5_config_types.hpp"
#include "services/interfaces/workflow/gta5/gta5_frame_cost.hpp"
#include "services/interfaces/workflow/gta5/gta5_geometry.hpp"
#include "services/interfaces/workflow/gta5/gta5_instance_batch.hpp"
#include "services/interfaces/workflow/gta5/gta5_load_pool.hpp"
#include "services/interfaces/workflow/gta5/gta5_resident_tile.hpp"
#include "services/interfaces/workflow/gta5/gta5_resource_cache.hpp"
#include "services/interfaces/workflow/gta5/gta5_texture_cache.hpp"
#include "services/interfaces/workflow/gta5/gta5_tile_coord.hpp"
#include "services/interfaces/workflow/gta5/gta5_vehicle_types.hpp"

#include <glm/glm.hpp>
#include <future>
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace sdl3cpp::services::impl {

/// Shared by the gta5.* steps. Owned by the registrar and injected into
/// each step rather than copied through the workflow context: it holds
/// the geometry cache and is far too large to round-trip per frame.
struct Gta5StreamState {
    Gta5WorldConfig world;
    Gta5StreamingConfig streaming;

    std::unordered_map<Gta5TileCoord, Gta5ResidentTile, Gta5TileCoordHash>
        resident;
    std::unordered_map<std::string, Gta5Geometry> geometryCache;
    /// Textures outlive individual archetypes on purpose: one is
    /// typically shared across a whole district.
    Gta5TextureCache textureCache;

    /// Written by gta5.tiles.resolve, consumed by load and evict in the
    /// same frame so all three agree on one centre.
    std::unordered_set<Gta5TileCoord, Gta5TileCoordHash> wanted;
    Gta5TileCoord centre{0, 0};
    glm::vec3 centreOrigin{0.f};

    /// Cars. Not part of any tile: they belong to the physics world and
    /// must never be streamed out from under it.
    std::vector<Gta5Vehicle> vehicles;
    /// Index into `vehicles` the player is sitting in, or -1 on foot.
    int seated{-1};
    /// The player's body in third person: drawn like the cars, never culled.
    std::vector<Gta5Instance> character;

    /// Tiles whose band changed; evict tears them down, load rebuilds.
    std::unordered_set<Gta5TileCoord, Gta5TileCoordHash> rebuild;

    /// The map, indexed in memory off-thread, and files opened lately.
    std::shared_ptr<const Gta5AssetIndex> assets;
    std::future<std::shared_ptr<const Gta5AssetIndex>> assetsPending;
    Gta5ResourceCache resources;
    /// Workers preparing archetypes, and prepared ones waiting on a
    /// texture another job is still reading. Declared after `resources`,
    /// which the workers use, so the pool is destroyed -- and its threads
    /// joined -- first.
    std::unique_ptr<Gta5LoadPool> pool;
    std::vector<Gta5PreparedGeometry> waiting;

    /// Archetypes already reported missing, so the log says it once.
    std::unordered_set<std::string> reportedMissing;

    Gta5GeometryArena arena;  // every map mesh's vertices and indices
    Gta5UploadBatch uploads;  // staged GPU copies: one submit a frame
    SDL_GPUSampler* textureSampler{nullptr};  // shared by map textures
    /// Last instance count logged, so a steady frame stays quiet.
    int lastDrawLogged{-1};
    Gta5InstanceBatch batch;  // this frame's visible instances, culled
    Gta5FrameCost cost;  // CPU time this frame; see gta5.frame.stats
    int textureBinds{0};
};

}  // namespace sdl3cpp::services::impl
