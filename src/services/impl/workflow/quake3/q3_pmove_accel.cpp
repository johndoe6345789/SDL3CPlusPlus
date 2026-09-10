#include "services/interfaces/workflow/quake3/q3_pmove.hpp"

#include "services/interfaces/workflow/quake3/q3_slide_planes.hpp"
#include "services/interfaces/workflow/quake3/q3_wish_dir.hpp"

#include <algorithm>
#include <cmath>

namespace sdl3cpp::q3 {

void PmFriction(services::impl::Q3PlayerState& ps, float dt) {
    if (!ps.onGround) return;

    const float speed = std::sqrt(ps.velocity.x * ps.velocity.x +
                                  ps.velocity.z * ps.velocity.z);
    if (speed < 0.1f) {
        ps.velocity.x = 0.0f;
        ps.velocity.z = 0.0f;
        return;
    }

    const float control  = std::max(speed, kStopSpeed);
    const float drop     = control * kFriction * dt;
    const float newSpeed = std::max(0.0f, speed - drop);

    const float scale = newSpeed / speed;
    ps.velocity.x *= scale;
    ps.velocity.z *= scale;
}

void PmAccelerate(services::impl::Q3PlayerState& ps, const Q3UserCmd& cmd,
                  float dt) {
    const auto wish =
        ComputeWish(cmd.forwardMove, cmd.rightMove, cmd.yaw, kMaxSpeed);
    if (wish.speed <= 0.0f) return;

    // ioq3 PM_WalkMove projects the movement basis onto the ground plane
    // before accelerating, so on a slope "forward" means up the slope.
    // Without it the wish stays horizontal, the mover pushes into the
    // surface instead of along it, and only the step machinery gets them
    // up — which is why walking up a slope used to stall where running
    // up the same slope did not.
    glm::vec3 wishDir = wish.direction;
    if (ps.onGround) {
        const glm::vec3 onPlane =
            ClipVelocity(wishDir, ps.groundNormal, kOverclip);
        const float length = glm::length(onPlane);
        if (length > 0.001f) {
            wishDir = onPlane / length;
        }
    }

    const float accel        = ps.onGround ? kAccelerate : kAirAccelerate;
    const float currentSpeed = glm::dot(ps.velocity, wishDir);
    const float addSpeed     = wish.speed - currentSpeed;
    if (addSpeed <= 0.0f) return;

    ps.velocity += wishDir * std::min(addSpeed, accel * wish.speed * dt);
}

}  // namespace sdl3cpp::q3
