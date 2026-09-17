#pragma once

#include "services/interfaces/workflow/fs2024/building/fs2024_polygon.hpp"
#include "services/interfaces/workflow/fs2024/prepare/texture/fs2024_ground_image.hpp"

#include <optional>
#include <string>
#include <vector>

namespace sdl3cpp::tools::fs2024 {

struct RunwayInfo {
    float x = 0.f, z = 0.f, heading = 0.f, length = 0.f, width = 0.f;
    int number = 0;
};

struct BuildingFootprint {
    std::vector<sdl3cpp::services::impl::Point2> footprint;  ///< engine
                                                             ///< (x, z) m
    float height = 0.f;
};

/// Slices `heights` (a `cells` x `cells` grid, row-major, at `spacing`
/// metres, starting at (originX, originZ)) and `ground` into the tile
/// grid under `outDir/tiles/<tx>_<tz>/`, matching exactly what
/// fs2024.tiles.load reads. `tileSize` and `cellsPerTile` must be the
/// same ones ComputeGridLayout produced for this bake -- passing a
/// mismatched pair reintroduces the exact key-drift bug grid_layout
/// exists to prevent. Buildings are split into whichever tile their
/// footprint overlaps (rare to span more than one, but a large block
/// can); returns how many tiles were written.
int WriteTiles(const std::string& outDir, const std::vector<float>& heights,
              int cells, float spacing, float originX, float originZ,
              float tileSize, int cellsPerTile, const GroundImage& ground,
              const std::vector<BuildingFootprint>& buildings,
              const std::optional<RunwayInfo>& runway);

}  // namespace sdl3cpp::tools::fs2024
