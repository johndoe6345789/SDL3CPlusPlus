#include "services/interfaces/workflow/input/input_axis_deadzone.hpp"

#include <algorithm>
#include <cmath>

namespace sdl3cpp::services::impl {

float ApplyAxisDeadzone(float value, float deadzone) {
    float clamped = std::max(-1.0f, std::min(1.0f, value));
    if (std::abs(clamped) < deadzone) {
        return 0.0f;
    }
    if (clamped > 0.0f) {
        return (clamped - deadzone) / (1.0f - deadzone);
    }
    return (clamped + deadzone) / (1.0f - deadzone);
}

}  // namespace sdl3cpp::services::impl
