#pragma once

#include "services/interfaces/workflow/fs2024/building/fs2024_polygon.hpp"
#include "services/interfaces/workflow/fs2024/building/fs2024_roof_mesh.hpp"
#include "services/interfaces/workflow/fs2024/data/cgl/fs2024_bld_tile.hpp"

#include <vector>

namespace sdl3cpp::services::impl {

/// One building to raise, in its tile's local metres.
struct Fs2024BuildingPlan {
    std::vector<Point2> footprint;  ///< outer ring, x east, y south
    float height = 0.f;             ///< eaves above the ground under it
    RoofShape roof = RoofShape::Flat;
    float roofRise = 0.f;
    bool roofSurveyed = false;      ///< a data set said what the roof is
};

/// Every building FS2024 stores for one level-14 tile, from its own two
/// data sets as ReadTile returns them (surveyed `bldo` first, then the
/// imagery-derived `bldn`). The surveyed set wins on outline and
/// storeys; where the imagery set has the same building (within a few
/// metres) and the surveyed one never recorded a roof, it lends its
/// roof. Only buildings whose centre is inside this tile are kept, so
/// one straddling an edge is raised once, not by both neighbours.
std::vector<Fs2024BuildingPlan> PlanFs2024TileBuildings(
    const std::vector<sdl3cpp::fs2024::BldTile>& tiles, float tileSize);

}  // namespace sdl3cpp::services::impl
