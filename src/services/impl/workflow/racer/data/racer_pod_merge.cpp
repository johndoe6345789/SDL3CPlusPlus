#include "racer_pod_parts.hpp"

namespace sdl3cpp::services::impl::racer_model_detail {

PodPartBounds BoundsOf(const RacerModel& part) {
    PodPartBounds b;
    for (const auto& batch : part.batches) {
        for (const auto& v : batch.vertices) {
            const glm::vec3 p = kPodPartScale * glm::vec3(v.x, v.y, v.z);
            b.lo = glm::min(b.lo, p);
            b.hi = glm::max(b.hi, p);
        }
    }
    return b;
}

void MoveAndMerge(RacerModel& part, const glm::vec3& by, RacerModel& into) {
    for (auto& batch : part.batches) {
        for (auto& v : batch.vertices) {
            v.x = kPodPartScale * v.x + by.x;
            v.y = kPodPartScale * v.y + by.y;
            v.z = kPodPartScale * v.z + by.z;
        }
        into.batches.push_back(std::move(batch));
    }
    into.meshCount += part.meshCount;
    into.triangleCount += part.triangleCount;
}

}  // namespace sdl3cpp::services::impl::racer_model_detail
