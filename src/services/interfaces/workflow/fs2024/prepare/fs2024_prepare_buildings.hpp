#pragma once

#include "services/interfaces/workflow/fs2024/prepare/fs2024_local_frame.hpp"
#include "services/interfaces/workflow/fs2024/prepare/fs2024_osm_json.hpp"
#include "services/interfaces/workflow/fs2024/prepare/fs2024_tile_writer.hpp"

namespace sdl3cpp::tools::fs2024 {

/// OSM building footprints converted to engine (x, z) metres, ready
/// for WriteTiles to split across the tile grid.
std::vector<BuildingFootprint> ConvertBuildings(
    const std::vector<OsmWay>& buildings, const LocalFrame& frame);

}  // namespace sdl3cpp::tools::fs2024
