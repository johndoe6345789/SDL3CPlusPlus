#pragma once

#include "services/interfaces/workflow/fs2024/prepare/landmark/fs2024_landmark_placement.hpp"
#include "services/interfaces/workflow/fs2024/prepare/fs2024_tile_writer.hpp"

#include <vector>

namespace sdl3cpp::fs2024 {

/// Removes every generated building standing where a real landmark
/// model does, one radius per landmark (see fs2024_landmark_bounds).
/// A building counts as under the landmark when its own centre is
/// inside that radius. These models are wide -- the Palace of
/// Westminster's is 464 m across, grounds included -- so testing a
/// corner instead would delete whole streets that merely touch the
/// edge of one.
void DropBuildingsUnderLandmarks(
    const std::vector<LandmarkInstance>& landmarks,
    const std::vector<float>& radii,
    std::vector<BuildingFootprint>& buildings);

}  // namespace sdl3cpp::fs2024
