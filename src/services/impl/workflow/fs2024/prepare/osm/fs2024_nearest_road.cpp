#include "services/interfaces/workflow/fs2024/prepare/osm/fs2024_nearest_road.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <utility>

namespace sdl3cpp::fs2024 {
namespace {

// Best (most road-like) first, matching python/fs2024/osm_roads.py's
// own priority list.
constexpr std::array<const char*, 9> kPriority{
    "primary",     "secondary",     "tertiary", "trunk",  "unclassified",
    "residential", "living_street", "service",  "footway"};

int Priority(const OsmWay& road) {
    for (std::size_t i = 0; i < kPriority.size(); ++i) {
        if (road.type == kPriority[i]) return static_cast<int>(i);
    }
    return static_cast<int>(kPriority.size());
}

/// Nearest point on segment ab to p, as (x, y, distance-squared).
std::array<double, 3> ClosestOnSegment(double px, double py, double ax,
                                       double ay, double bx, double by) {
    const double dx = bx - ax, dy = by - ay;
    const double length2 = dx * dx + dy * dy;
    double t = length2 == 0.0
                  ? 0.0
                  : std::clamp(((px - ax) * dx + (py - ay) * dy) / length2,
                              0.0, 1.0);
    const double x = ax + dx * t, y = ay + dy * t;
    const double ddx = px - x, ddy = py - y;
    return {x, y, ddx * ddx + ddy * ddy};
}

}  // namespace

std::optional<NearestRoad> FindNearestRoad(double lat, double lon,
                                           const std::vector<OsmWay>& roads) {
    const double cosLat = std::cos(lat * 3.14159265358979323846 / 180.0);
    std::optional<NearestRoad> best;
    std::pair<int, double> bestKey{1 << 30, 1e30};

    for (const OsmWay& road : roads) {
        for (std::size_t i = 0; i + 1 < road.points.size(); ++i) {
            const auto& [ax, ay] = road.points[i];
            const auto& [bx, by] = road.points[i + 1];
            const auto [x, y, dist2] = ClosestOnSegment(lon, lat, ax, ay,
                                                        bx, by);
            const double distMetres =
                std::sqrt(dist2) * 111320.0 * std::max(cosLat, 0.01);
            const std::pair<int, double> key{Priority(road), distMetres};
            if (key < bestKey) {
                bestKey = key;
                const double dx = bx - ax, dy = by - ay;
                const double heading =
                    std::atan2(dx * cosLat, dy) * 180.0 / 3.14159265358979323846;
                best = NearestRoad{&road, x, y,
                                   static_cast<float>(
                                       std::fmod(heading + 360.0, 360.0))};
            }
        }
    }
    return best;
}

}  // namespace sdl3cpp::fs2024
