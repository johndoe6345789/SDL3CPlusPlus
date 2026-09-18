#include "services/interfaces/workflow/fs2024/building/fs2024_convex_hull.hpp"

#include <algorithm>

namespace sdl3cpp::services::impl {
namespace {

float Cross(const Point2& o, const Point2& a, const Point2& b) {
    return (a.x - o.x) * (b.y - o.y) - (a.y - o.y) * (b.x - o.x);
}

void BuildChain(const std::vector<Point2>& sorted,
                std::vector<Point2>& hull) {
    for (const Point2& point : sorted) {
        while (hull.size() >= 2 &&
               Cross(hull[hull.size() - 2], hull.back(), point) <= 0.f) {
            hull.pop_back();
        }
        hull.push_back(point);
    }
}

}  // namespace

std::vector<Point2> ConvexHull(std::vector<Point2> points) {
    std::sort(points.begin(), points.end(),
             [](const Point2& a, const Point2& b) {
                 return a.x != b.x ? a.x < b.x : a.y < b.y;
             });
    points.erase(std::unique(points.begin(), points.end(),
                            [](const Point2& a, const Point2& b) {
                                return a.x == b.x && a.y == b.y;
                            }),
                points.end());
    if (points.size() < 3) return points;

    std::vector<Point2> lower, upper;
    BuildChain(points, lower);
    std::reverse(points.begin(), points.end());
    BuildChain(points, upper);
    lower.pop_back();
    upper.pop_back();
    lower.insert(lower.end(), upper.begin(), upper.end());
    return lower;
}


}  // namespace sdl3cpp::services::impl
