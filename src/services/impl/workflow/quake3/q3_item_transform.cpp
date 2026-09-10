#include "services/interfaces/workflow/quake3/q3_item_transform.hpp"

#include "services/interfaces/workflow/quake3/q3_axes.hpp"
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
    return q3::PlaceModel(pos, yaw);
}

}  // namespace sdl3cpp::services::impl
