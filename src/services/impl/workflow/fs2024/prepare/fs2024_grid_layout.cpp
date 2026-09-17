#include "services/interfaces/workflow/fs2024/prepare/fs2024_grid_layout.hpp"

#include <algorithm>
#include <cmath>

namespace sdl3cpp::tools::fs2024 {

GridLayout ComputeGridLayout(float extent, float requestedTileSize,
                             float spacing, int minRadiusTiles) {
    const int cellsPerTile =
        static_cast<int>(std::lround(requestedTileSize / spacing));
    const float tileSize = static_cast<float>(cellsPerTile) * spacing;
    const int radius = std::max(
        minRadiusTiles,
        static_cast<int>(std::lround(extent / tileSize / 2.f)));
    const int tilesPerSide = 2 * radius + 1;
    const float origin = -static_cast<float>(radius) * tileSize;

    GridLayout layout;
    layout.tileSize = tileSize;
    layout.originX = origin;
    layout.originZ = origin;
    layout.extent = static_cast<float>(tilesPerSide) * tileSize;
    layout.cells = tilesPerSide * cellsPerTile + 1;
    return layout;
}

}  // namespace sdl3cpp::tools::fs2024
