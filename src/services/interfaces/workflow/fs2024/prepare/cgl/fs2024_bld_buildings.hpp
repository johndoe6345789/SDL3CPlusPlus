#pragma once

#include "services/interfaces/workflow/fs2024/data/fs2024_local_frame.hpp"
#include "services/interfaces/workflow/fs2024/prepare/fs2024_tile_writer.hpp"

#include <string>
#include <vector>

namespace sdl3cpp::fs2024 {

/// Every building FS2024's own data has within `extent` metres of
/// `frame`'s origin, as engine-space footprints with their real
/// storey count, roof shape and roof ridge. Reads the game's
/// `fs-base-cgl` building library directly -- these are the same
/// footprints, storeys and roof types the simulator feeds to its
/// procedural building generator.
std::vector<BuildingFootprint> ReadFs2024Buildings(
    const std::string& cglRoot, const LocalFrame& frame, float extent);

}  // namespace sdl3cpp::fs2024
