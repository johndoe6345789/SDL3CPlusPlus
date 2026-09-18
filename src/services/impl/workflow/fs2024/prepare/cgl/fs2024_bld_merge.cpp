#include "services/interfaces/workflow/fs2024/prepare/cgl/fs2024_bld_merge.hpp"

#include <cmath>

namespace sdl3cpp::fs2024 {
namespace {

using sdl3cpp::services::impl::Point2;

constexpr float kSameSpotMetres = 8.f;

}  // namespace

Point2 FootprintCentre(const std::vector<Point2>& ring) {
    Point2 sum;
    for (const Point2& point : ring) {
        sum.x += point.x / ring.size();
        sum.y += point.y / ring.size();
    }
    return sum;
}

BuildingFootprint* BuildingStandingHere(
    std::vector<BuildingFootprint>& existing, const Point2& centre) {
    for (BuildingFootprint& building : existing) {
        const Point2 other = FootprintCentre(building.footprint);
        if (std::abs(other.x - centre.x) < kSameSpotMetres &&
            std::abs(other.y - centre.y) < kSameSpotMetres) {
            return &building;
        }
    }
    return nullptr;
}

}  // namespace sdl3cpp::fs2024
