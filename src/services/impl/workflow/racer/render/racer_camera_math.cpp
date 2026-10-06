#include "services/interfaces/workflow/racer/render/racer_camera_math.hpp"

#include <algorithm>
#include <cmath>

namespace sdl3cpp::services::impl {

float RacerAngleDelta(float from, float to) {
    constexpr float kPi = 3.14159265f;
    float delta = std::fmod(to - from + kPi, 2.f * kPi);
    if (delta < 0.f) delta += 2.f * kPi;
    return delta - kPi;
}

float RacerCameraClearance(const RacerWorldState& state,
                           const glm::vec3& pod, const glm::vec3& eye) {
    float lowest = -1e9f;
    for (int k = 1; k <= 8; ++k) {
        const glm::vec3 p = pod + (eye - pod) * (k / 8.f);
        const auto ground =
            RacerGroundHeight(state.ground, p.x, p.z, p.y + 30.f);
        if (ground) lowest = std::max(lowest, *ground + 1.5f);
    }
    return lowest;
}

}  // namespace sdl3cpp::services::impl
