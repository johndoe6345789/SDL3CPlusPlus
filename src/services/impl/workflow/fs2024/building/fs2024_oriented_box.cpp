#include "services/interfaces/workflow/fs2024/building/fs2024_oriented_box.hpp"

#include "services/interfaces/workflow/fs2024/building/fs2024_convex_hull.hpp"

#include <algorithm>
#include <cmath>

namespace sdl3cpp::services::impl {
OrientedBox ComputeOrientedBox(const std::vector<Point2>& footprint) {
    const std::vector<Point2> hull = ConvexHull(footprint);
    OrientedBox best;
    if (hull.empty()) return best;

    float bestArea = -1.f;
    for (std::size_t i = 0; i < hull.size(); ++i) {
        const Point2& a = hull[i];
        const Point2& b = hull[(i + 1) % hull.size()];
        const float length = std::hypot(b.x - a.x, b.y - a.y);
        if (length < 1e-6f) continue;
        const Point2 axis{(b.x - a.x) / length, (b.y - a.y) / length};
        float minU = 1e30f, maxU = -1e30f, minV = 1e30f, maxV = -1e30f;
        for (const Point2& p : hull) {
            const float u = p.x * axis.x + p.y * axis.y;
            const float v = -p.x * axis.y + p.y * axis.x;
            minU = std::min(minU, u); maxU = std::max(maxU, u);
            minV = std::min(minV, v); maxV = std::max(maxV, v);
        }
        const float area = (maxU - minU) * (maxV - minV);
        if (bestArea >= 0.f && area >= bestArea) continue;
        bestArea = area;
        const float midU = (minU + maxU) / 2.f, midV = (minV + maxV) / 2.f;
        best.centre = {midU * axis.x - midV * axis.y,
                      midU * axis.y + midV * axis.x};
        best.longAxis = axis;
        best.halfLength = (maxU - minU) / 2.f;
        best.halfWidth = (maxV - minV) / 2.f;
    }
    if (best.halfWidth > best.halfLength) {
        best.longAxis = {-best.longAxis.y, best.longAxis.x};
        std::swap(best.halfLength, best.halfWidth);
    }
    return best;
}

}  // namespace sdl3cpp::services::impl
