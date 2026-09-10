#include "services/interfaces/workflow/quake3/q3_item_transform.hpp"

#include "services/interfaces/workflow/quake3/q3_pm_constants.hpp"

#include <glm/gtc/matrix_transform.hpp>

#include <cmath>

namespace sdl3cpp::services::impl {
namespace {

constexpr float kPi = 3.14159265358979f;

}  // namespace

float Q3ItemBobHeight(float timeSeconds, int index) {
    const float ms   = timeSeconds * 1000.0f;
    const float rate = 0.005f + static_cast<float>(index) * 0.00001f;
    const float bob  = 4.0f + std::cos((ms + 1000.0f) * rate) * 4.0f;
    return q3::FromQuakeUnits(bob);
}

float Q3ItemSpinYaw(float timeSeconds, bool fast) {
    const float period = fast ? 1.024f : 2.048f;
    const float phase  = std::fmod(timeSeconds, period) / period;
    return phase * 2.0f * kPi;
}

glm::mat4 Q3ItemMatrix(const glm::vec3& pos, float yaw) {
    const glm::vec3 f(-std::sin(yaw), 0.f, -std::cos(yaw));
    const glm::vec3 u(0.f, 1.f, 0.f);
    const glm::vec3 r = glm::cross(u, f);
    glm::mat4 orient(1.0f);
    orient[0] = glm::vec4(f, 0.0f);
    orient[1] = glm::vec4(r, 0.0f);
    orient[2] = glm::vec4(u, 0.0f);
    return glm::translate(glm::mat4(1.0f), pos) * orient;
}

}  // namespace sdl3cpp::services::impl
