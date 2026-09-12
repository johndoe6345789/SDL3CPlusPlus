#include "services/interfaces/workflow/gta5/gta5_ped_stance.hpp"

#include <algorithm>
#include <cmath>

namespace sdl3cpp::services::impl {

void SettleGta5Stance(Gta5Stance& stance, WorkflowContext& context,
                      const glm::vec2& run, float dt) {
    constexpr float kPi = 3.14159265f;
    const bool armed =
        !context.GetString("gta5.weapon.model", "").empty() &&
        !context.GetBool("gta5.menu.open", false);
    const float speed = glm::length(run);
    float want = stance.yaw;
    if (armed) {
        // The ped looks along its own +z and camera_yaw counts from
        // -z, so the two are half a turn apart.
        want = context.Get<float>("camera_yaw", 0.f) + kPi;
    } else if (speed > 0.5f) {
        want = std::atan2(run.x, run.y);
    }
    const float turn = std::remainder(want - stance.yaw, 2.f * kPi);
    stance.yaw += turn * std::min(1.f, dt * (armed ? 16.f : 10.f));
    stance.aim +=
        ((armed ? 1.f : 0.f) - stance.aim) * std::min(1.f, dt * 8.f);
}

}  // namespace sdl3cpp::services::impl
