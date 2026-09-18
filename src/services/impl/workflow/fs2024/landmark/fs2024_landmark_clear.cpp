#include "services/interfaces/workflow/fs2024/landmark/fs2024_landmark_clear.hpp"

#include "services/interfaces/workflow/fs2024/build/fs2024_building_spot.hpp"

#include <algorithm>

namespace sdl3cpp::services::impl {
namespace {

constexpr float kClearsBelowMetres = 6.f;

bool Under(const Fs2024LandmarkInstance& instance,
           const Fs2024LandmarkKitGpu& kit, const Point2& centre) {
    const glm::vec4 local = glm::inverse(instance.model) *
                            glm::vec4(centre.x, 0.f, centre.y, 1.f);
    return local.x >= kit.min.x && local.x <= kit.max.x &&
           local.z >= kit.min.z && local.z <= kit.max.z;
}

}  // namespace

void DropFs2024BuildingsUnderLandmarks(
    std::vector<Fs2024BuildingPlan>& plans,
    const std::vector<Fs2024LandmarkInstance>& instances,
    const Fs2024LandmarkKits& kits) {
    for (const Fs2024LandmarkInstance& instance : instances) {
        const auto kit = kits.find(instance.entry.name);
        if (kit == kits.end()) continue;
        if (kit->second.max.y - kit->second.min.y < kClearsBelowMetres) {
            continue;
        }
        plans.erase(std::remove_if(plans.begin(), plans.end(),
                                   [&](const Fs2024BuildingPlan& plan) {
                                       return Under(
                                           instance, kit->second,
                                           Fs2024FootprintCentre(
                                               plan.footprint));
                                   }),
                    plans.end());
    }
}

}  // namespace sdl3cpp::services::impl
