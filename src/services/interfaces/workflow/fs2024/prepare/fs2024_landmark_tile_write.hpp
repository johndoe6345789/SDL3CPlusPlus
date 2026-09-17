#pragma once

#include "services/interfaces/workflow/fs2024/prepare/fs2024_landmark_placement.hpp"

#include <string>

namespace sdl3cpp::tools::fs2024 {

/// Writes each instance's `landmarks.json` into whichever tile
/// directory under `outDir` its (x, z) falls in (creating the
/// directory is `WriteTiles`'s job, done first, same as for
/// buildings/roads). A landmark this small a bake ever places is
/// never expected to straddle a tile boundary the way a runway does,
/// so unlike `WriteTiles`'s buildings this is a single-tile lookup,
/// not an overlap test.
void WriteLandmarkInstances(const std::string& outDir,
                           const std::vector<LandmarkInstance>& instances,
                           float tileSize);

}  // namespace sdl3cpp::tools::fs2024
