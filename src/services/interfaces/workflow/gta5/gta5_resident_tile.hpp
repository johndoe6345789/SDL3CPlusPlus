#pragma once

#include "services/interfaces/workflow/gta5/gta5_geometry.hpp"
#include "services/interfaces/workflow/gta5/gta5_lod.hpp"
#include "services/interfaces/workflow/gta5/gta5_placement.hpp"

#include <cstddef>
#include <future>
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
    /// Its archetypes have been handed to the load pool at bandAtSpawn.
    bool prefetched{false};
    /// Its placements, being read on a thread of their own.
    std::future<std::vector<Gta5Placement>> reading;
};

}  // namespace sdl3cpp::services::impl
