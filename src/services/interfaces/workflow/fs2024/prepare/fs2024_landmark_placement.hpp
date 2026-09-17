#pragma once

#include "services/interfaces/workflow/fs2024/prepare/fs2024_landmark_catalog.hpp"
#include "services/interfaces/workflow/fs2024/prepare/fs2024_local_frame.hpp"
#include "services/interfaces/workflow/fs2024/prepare/fs2024_osm_json.hpp"

#include <string>
#include <vector>

namespace sdl3cpp::tools::fs2024 {

/// A landmark's placement in engine space: its OSM footprint's own
/// centroid (the mean of its points -- a real landmark's footprint is
/// rarely convex enough for a fancier centroid to matter more than
/// simplicity does), and the catalog's manual heading.
struct LandmarkInstance {
    std::string model;
    float x = 0.f, z = 0.f;
    float headingDegrees = 0.f;
};

/// Matches each catalog entry against `buildings` by a case-insensitive
/// substring of its OSM `name`, first match wins. A catalog entry with
/// no match in this bake's OSM data is simply skipped.
std::vector<LandmarkInstance> MatchLandmarks(
    const std::vector<OsmWay>& buildings,
    const std::vector<LandmarkCatalogEntry>& catalog,
    const LocalFrame& frame);

}  // namespace sdl3cpp::tools::fs2024
