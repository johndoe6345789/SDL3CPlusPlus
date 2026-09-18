#include "services/interfaces/workflow/fs2024/assemble/fs2024_building_spot.hpp"

#include <cmath>

namespace sdl3cpp::services::impl {
namespace {

constexpr float kSameSpotMetres = 8.f;

}  // namespace

Point2 Fs2024FootprintCentre(const std::vector<Point2>& ring) {
    Point2 sum;
    for (const Point2& p : ring) {
        sum.x += p.x / ring.size();
        sum.y += p.y / ring.size();
    }
    return sum;
}

Fs2024BuildingPlan* Fs2024PlanStandingAt(std::vector<Fs2024BuildingPlan>& plans,
                                         const Point2& centre) {
    for (Fs2024BuildingPlan& plan : plans) {
        const Point2 other = Fs2024FootprintCentre(plan.footprint);
        if (std::abs(other.x - centre.x) < kSameSpotMetres &&
            std::abs(other.y - centre.y) < kSameSpotMetres) {
            return &plan;
        }
    }
    return nullptr;
}

}  // namespace sdl3cpp::services::impl
