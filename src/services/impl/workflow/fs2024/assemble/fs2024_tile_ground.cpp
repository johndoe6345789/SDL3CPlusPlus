#include "services/interfaces/workflow/fs2024/assemble/fs2024_tile_ground.hpp"

#include <algorithm>

namespace sdl3cpp::services::impl {
Fs2024Heightfield BuildFs2024TileHeights(Fs2024DemSampler& dem, int quadX,
                                         int quadY, int level, float span,
                                         int cells) {
    const double kTiles = static_cast<double>(1 << level);
    Fs2024Heightfield field;
    field.columns = field.rows = cells + 1;
    field.spacing = span / static_cast<float>(cells);
    field.origin = glm::vec2(0.f);
    field.heights.resize(static_cast<std::size_t>(field.columns) *
                         field.rows);
    for (int row = 0; row < field.rows; ++row) {
        const double v = (quadY + static_cast<double>(row) / cells) / kTiles;
        for (int column = 0; column < field.columns; ++column) {
            const double u =
                (quadX + static_cast<double>(column) / cells) / kTiles;
            field.heights[static_cast<std::size_t>(row) * field.columns +
                          column] = dem.MetresAt(u, v);
        }
    }
    const auto [lo, hi] =
        std::minmax_element(field.heights.begin(), field.heights.end());
    field.minHeight = *lo;
    field.maxHeight = *hi;
    return field;
}

std::vector<std::uint8_t> BuildFs2024TileClasses(Fs2024ClassSampler& classes,
                                                 int quadX, int quadY,
                                                 int level, int size) {
    const double kTiles = static_cast<double>(1 << level);
    std::vector<std::uint8_t> out(static_cast<std::size_t>(size) * size);
    for (int row = 0; row < size; ++row) {
        const double v = (quadY + (row + 0.5) / size) / kTiles;
        for (int column = 0; column < size; ++column) {
            const double u = (quadX + (column + 0.5) / size) / kTiles;
            out[static_cast<std::size_t>(row) * size + column] =
                classes.ClassAt(u, v);
        }
    }
    return out;
}

}  // namespace sdl3cpp::services::impl
