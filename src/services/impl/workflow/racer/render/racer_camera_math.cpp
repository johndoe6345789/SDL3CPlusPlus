#include "services/interfaces/workflow/racer/render/racer_camera_math.hpp"

#include <glm/gtc/type_ptr.hpp>

#include <algorithm>
#include <cmath>

namespace sdl3cpp::services::impl {
namespace {

constexpr float kMaxRise = 25.f;  // metres over the pod, at most

}  // namespace

nlohmann::json RacerMatrixJson(const glm::mat4& matrix) {
    nlohmann::json out = nlohmann::json::array();
    const float* values = glm::value_ptr(matrix);
    for (int i = 0; i < 16; ++i) out.push_back(values[i]);
    return out;
}

float RacerAngleDelta(float from, float to) {
    constexpr float kPi = 3.14159265f;
    float delta = std::fmod(to - from + kPi, 2.f * kPi);
    if (delta < 0.f) delta += 2.f * kPi;
    return delta - kPi;
}

float RacerCameraClearance(const RacerWorldState& state,
                           const glm::vec3& pod, const glm::vec3& eye) {
    // The sight line from the pod back to the eye must clear the ground
    // everywhere on the way: at a share t of the way, the eye must sit
    // high enough that the line there is 1.5 m above it. Capped, so a
    // pod in a deep dip is followed from above rather than from orbit.
    float lowest = -1e9f;
    for (int k = 1; k <= 8; ++k) {
        const float t = k / 8.f;
        const glm::vec3 p = pod + (eye - pod) * t;
        const auto ground =
            RacerGroundHeight(state.ground, p.x, p.z, p.y + 30.f);
        if (!ground) continue;
        const float need = pod.y + (*ground + 1.5f - pod.y) / t;
        lowest = std::max(lowest, std::min(need, pod.y + kMaxRise));
    }
    return lowest;
}

glm::vec3 RacerCameraPullIn(const RacerWorldState& state,
                            const glm::vec3& focus, const glm::vec3& eye,
                            float nearest) {
    // Short steps, as the wall grid only looks up the cells at the ends.
    constexpr int kSteps = 8;
    for (int k = 0; k < kSteps; ++k) {
        const glm::vec3 a = focus + (eye - focus) * (k / float(kSteps));
        const glm::vec3 b = focus + (eye - focus) * ((k + 1) / float(kSteps));
        if (RacerWallBetween(state.ground, a, b)) {
            // Stop one step short of the step that hits.
            const float span = glm::length(eye - focus);
            const float least = span > 0.f ? nearest / span : 1.f;
            const float keep = std::max(k - 1, 0) / float(kSteps);
            return focus + (eye - focus) * std::clamp(keep, least, 1.f);
        }
    }
    return eye;
}

}  // namespace sdl3cpp::services::impl
