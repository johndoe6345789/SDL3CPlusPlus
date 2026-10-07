#include "services/interfaces/workflow/racer/player/racer_line_guide.hpp"

#include <algorithm>
#include <cmath>

namespace sdl3cpp::services::impl {

void GuideRacerPodToLine(RacerPodState& pod,
                         const std::vector<glm::vec3>& lapPoints,
                         int nearestPoint, float maxOffset, float pullSpeed,
                         float dt) {
    const int count = static_cast<int>(lapPoints.size());
    if (count < 2 || nearestPoint < 0) return;
    const glm::vec3& here = lapPoints[nearestPoint % count];
    glm::vec3 along(0.f);
    for (int k = 1; k < count && glm::length(along) < 3.f; ++k) {
        along = lapPoints[(nearestPoint + k) % count] - here;
        along.y = 0.f;
    }
    if (glm::length(along) < 1e-4f) return;
    along = glm::normalize(along);
    const glm::vec3 side(-along.z, 0.f, along.x);
    const float lateral = glm::dot(pod.position - here, side);
    // In the air an AI pod is brought back over the line firmly, so a
    // crest never drops it beside a narrow ledge.
    const float allowed = pod.grounded ? maxOffset : 0.4f * maxOffset;
    const float rate = pod.grounded ? pullSpeed : 2.f * pullSpeed;
    const float excess = std::fabs(lateral) - allowed;
    if (excess <= 0.f) return;
    const float pull = std::min(excess, rate * dt);
    pod.position -= side * (lateral > 0.f ? pull : -pull);
}

}  // namespace sdl3cpp::services::impl
