#pragma once

#include "services/interfaces/workflow/switchback/terrain/switchback_heightmap.hpp"
#include "services/interfaces/workflow/switchback/track/switchback_track_road.hpp"

#include <vector>

namespace sdl3cpp::services::impl {

/// Flattens the ground under the road to the road's height. Full strength
/// within the half width, easing out to the ground over `bankM`.
void CarveRoadIntoHeights(SwitchbackHeightmap& map, float stepM, float extentM,
                          const std::vector<SwitchbackRoadPoint>& points,
                          float roadHalfWidthM, float bankM);

}  // namespace sdl3cpp::services::impl
