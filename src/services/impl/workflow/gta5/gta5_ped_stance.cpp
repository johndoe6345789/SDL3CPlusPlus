#include "services/interfaces/workflow/gta5/gta5_ped_stance.hpp"

#include <algorithm>
#include <cmath>

namespace sdl3cpp::services::impl {

void SettleGta5Stance(Gta5Stance& stance, WorkflowContext& context,
                      const glm::vec2& run, float dt, float base) {
    constexpr float kPi = 3.14159265f;
    // Carrying a gun is not aiming one: he only squares up and brings
    // it to bear while the trigger or the aim is held.
    const bool armed = context.GetBool("gta5.weapon.aiming", false);
    const float speed = glm::length(run);
    float want = stance.yaw;
    // Both headings are counted the way camera_yaw is, and the ped is
    // turned from wherever its bind pose already points.
    if (armed) {
        want = context.Get<float>("camera_yaw", 0.f) - base;
    } else if (speed > 0.5f) {
        want = std::atan2(-run.x, -run.y) - base;
    }
    const float turn = std::remainder(want - stance.yaw, 2.f * kPi);
    stance.yaw += turn * std::min(1.f, dt * (armed ? 16.f : 10.f));
    // A gun is brought up smartly and put down more gently. Running
    // the raise at a pace, rather than easing towards a target it only
    // ever approaches, gives it a length: about a fifth of a second up.
    // Smoothstep then starts and settles the arm instead of jerking it
    // into motion, so the hand -- and the gun in it -- sweeps up.
    const float pace = armed ? dt / 0.2f : -dt / 0.35f;
    stance.raise = std::clamp(stance.raise + pace, 0.f, 1.f);
    const float r = stance.raise;
    stance.aim = r * r * (3.f - 2.f * r);
}

}  // namespace sdl3cpp::services::impl
