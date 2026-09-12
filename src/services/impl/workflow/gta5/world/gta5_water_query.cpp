#include "services/interfaces/workflow/gta5/world/gta5_water.hpp"

#include <algorithm>

namespace sdl3cpp::services::impl {
namespace {

float Side(float ax, float ay, float bx, float by, float px, float py) {
    return (bx - ax) * (py - ay) - (by - ay) * (px - ax);
}

/// Inside the quad, or inside its triangle: the three corners but the
/// one its type leaves out (as BuildGta5WaterMesh draws it).
bool Covers(const Gta5WaterQuad& q, float x, float y) {
    if (x < q.minX || x > q.maxX || y < q.minY || y > q.maxY) return false;
    if (q.type < 1 || q.type > 4) return true;
    constexpr int kDropped[5] = {-1, 0, 3, 2, 1};
    const float cx[4] = {q.minX, q.maxX, q.maxX, q.minX};
    const float cy[4] = {q.minY, q.minY, q.maxY, q.maxY};
    const int d = kDropped[q.type];
    const int a = (d + 1) % 4, b = (d + 3) % 4, kept = (d + 2) % 4;
    // Same side of the cut as the corner opposite the one left out.
    const float here = Side(cx[a], cy[a], cx[b], cy[b], x, y);
    const float there = Side(cx[a], cy[a], cx[b], cy[b], cx[kept], cy[kept]);
    return here * there >= 0.f;
}

}  // namespace

bool Gta5WaterHeightAt(const std::vector<Gta5WaterQuad>& water, float x,
                       float y, float& height) {
    bool found = false;
    for (const Gta5WaterQuad& q : water) {
        if (!Covers(q, x, y) || (found && q.z <= height)) continue;
        height = q.z;
        found = true;
    }
    return found;
}

void RasterGta5WaterHeights(const std::vector<Gta5WaterQuad>& water,
                            const Gta5WaterGrid& grid,
                            std::vector<float>& heights) {
    heights.assign(std::size_t(grid.columns) * grid.rows, kGta5NoWater);
    const float cell = grid.width / static_cast<float>(grid.columns);
    for (const Gta5WaterQuad& q : water) {
        // The cells whose centres the quad's box covers, then the test.
        const int c0 = std::max(0, int((q.minX - grid.minX) / cell));
        const int c1 = std::min(grid.columns - 1,
                                int((q.maxX - grid.minX) / cell));
        const int r0 = std::max(0, int((grid.maxY - q.maxY) / cell));
        const int r1 =
            std::min(grid.rows - 1, int((grid.maxY - q.minY) / cell));
        for (int r = r0; r <= r1; ++r) {
            for (int c = c0; c <= c1; ++c) {
                const float x = grid.minX + (c + 0.5f) * cell;
                const float y = grid.maxY - (r + 0.5f) * cell;
                float& h = heights[std::size_t(r) * grid.columns + c];
                if (Covers(q, x, y) && q.z > h) h = q.z;
            }
        }
    }
}

}  // namespace sdl3cpp::services::impl
