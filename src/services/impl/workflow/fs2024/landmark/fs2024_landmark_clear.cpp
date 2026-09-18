#include "services/interfaces/workflow/fs2024/landmark/fs2024_landmark_clear.hpp"

#include "services/interfaces/workflow/fs2024/assemble/fs2024_building_spot.hpp"

#include <algorithm>

namespace sdl3cpp::services::impl {
namespace {

constexpr float kClearsBelowMetres = 6.f;

bool Under(const glm::mat4& inverse, const Fs2024LandmarkBounds& bounds,
           const Point2& centre) {
    const glm::vec4 local =
        inverse * glm::vec4(centre.x, 0.f, centre.y, 1.f);
    return local.x >= bounds.min.x && local.x <= bounds.max.x &&
           local.z >= bounds.min.z && local.z <= bounds.max.z;
}

}  // namespace

void DropFs2024BuildingsUnderLandmarks(
    std::vector<Fs2024BuildingPlan>& plans,
    const std::vector<Fs2024LandmarkInstance>& instances,
    const Fs2024LandmarkBoundsMap& bounds) {
    for (const Fs2024LandmarkInstance& instance : instances) {
        const auto found = bounds.find(instance.entry.name);
        if (found == bounds.end()) continue;
        const Fs2024LandmarkBounds& box = found->second;
        if (box.max.y - box.min.y < kClearsBelowMetres) continue;
        const glm::mat4 inverse = glm::inverse(instance.model);
        plans.erase(std::remove_if(plans.begin(), plans.end(),
                                   [&](const Fs2024BuildingPlan& plan) {
                                       return Under(inverse, box,
                                                    Fs2024FootprintCentre(
                                                        plan.footprint));
                                   }),
                    plans.end());
    }
}

}  // namespace sdl3cpp::services::impl
