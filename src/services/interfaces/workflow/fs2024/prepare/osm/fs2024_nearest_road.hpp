#pragma once

#include "services/interfaces/workflow/fs2024/data/fs2024_local_frame.hpp"
#include "services/interfaces/workflow/fs2024/prepare/osm/fs2024_osm_json.hpp"

#include <optional>

namespace sdl3cpp::fs2024 {

struct NearestRoad {
    const OsmWay* road = nullptr;
    double lon = 0.0, lat = 0.0;
    float headingDegrees = 0.f;
};

/// The best nearby road to (lat, lon): ranked by (highway priority,
/// distance) in that order, so a footway a metre away does not beat a
/// primary road ten metres away. Distances are converted to metres via
/// a flat-earth approximation, fine at the scale of "which of these
/// nearby roads."
std::optional<NearestRoad> FindNearestRoad(double lat, double lon,
                                           const std::vector<OsmWay>& roads);

}  // namespace sdl3cpp::fs2024
