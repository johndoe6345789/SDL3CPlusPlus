#pragma once

#include "services/interfaces/workflow/fs2024/data/cgl/fs2024_bld_tile.hpp"
#include "services/interfaces/workflow/fs2024/data/cgl/fs2024_quadkey.hpp"
#include "services/interfaces/workflow/fs2024/data/fs2024_local_frame.hpp"
#include "services/interfaces/workflow/fs2024/prepare/fs2024_tile_writer.hpp"

#include <vector>

namespace sdl3cpp::fs2024 {

/// Converts one decoded bld tile into engine-space buildings, keeping
/// only those within `extent` metres of the frame's origin and only
/// their outer rings (this engine's building mesh has no holes).
/// When `replaceExisting` is false the tile is treated as a fill-in
/// set: a building is skipped if one already in `out` stands on the
/// same spot, which is how the imagery-derived `bldn` set is meant to
/// sit under the surveyed `bldo` one.
void AppendBldBuildings(const BldTile& tile, const QuadTile& quad,
                       const LocalFrame& frame, float extent,
                       bool replaceExisting,
                       std::vector<BuildingFootprint>& out);

}  // namespace sdl3cpp::fs2024
