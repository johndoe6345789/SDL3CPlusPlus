#include "services/interfaces/workflow/racer/render/racer_camera_math.hpp"

#include <glm/gtc/type_ptr.hpp>

#include <cmath>

namespace sdl3cpp::services::impl {

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

}  // namespace sdl3cpp::services::impl
