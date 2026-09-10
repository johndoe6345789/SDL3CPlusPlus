#pragma once

#include "services/interfaces/workflow/gta5/gta5_config_types.hpp"
#include "services/interfaces/workflow/gta5/gta5_geometry.hpp"
#include "services/interfaces/workflow/gta5/gta5_placement.hpp"
#include "services/interfaces/workflow/gta5/gta5_tile_coord.hpp"

#include <cstddef>
#include <glm/glm.hpp>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace sdl3cpp::services::impl {

/// Bookkeeping for a tile that is loaded, or part-way through loading.
struct Gta5ResidentTile {
    std::vector<Gta5Placement> placements;
    /// Instances spawned so far, drawn by gta5.tiles.draw.
    std::vector<Gta5Instance> instances;
    /// How many placements have been spawned. Spawning is spread over
    /// frames, so a tile is only fully resident at placements.size().
    std::size_t spawnedCount{0};
    /// Band the spawned objects were built at. When a tile's band changes
    /// it is torn down and rebuilt, which is how LOD switching works.
    Gta5Lod bandAtSpawn{Gta5Lod::Hd};
    bool placementsRead{false};
};

/// Shared by the four gta5.* steps. Owned by the registrar and injected
/// into each step rather than copied through the workflow context: it
/// holds the geometry cache and is far too large to round-trip per frame.
struct Gta5StreamState {
    Gta5WorldConfig world;
    Gta5StreamingConfig streaming;

    std::unordered_map<Gta5TileCoord, Gta5ResidentTile, Gta5TileCoordHash>
        resident;
    std::unordered_map<std::string, Gta5Geometry> geometryCache;

    /// Written by gta5.tiles.resolve, consumed by load and evict in the
    /// same frame so all three agree on one centre.
    std::unordered_set<Gta5TileCoord, Gta5TileCoordHash> wanted;
    Gta5TileCoord centre{0, 0};
    glm::vec3 centreOrigin{0.f};

    /// Tiles whose band changed; evict tears them down, load rebuilds.
    std::unordered_set<Gta5TileCoord, Gta5TileCoordHash> rebuild;

    /// Archetypes already reported missing, so the log says it once.
    std::unordered_set<std::string> reportedMissing;

    /// Last draw count written to the log, so a steady frame stays quiet
    /// and only real changes are reported.
    int lastDrawLogged{-1};
};

}  // namespace sdl3cpp::services::impl
