#include "services/interfaces/workflow/quake3/workflow_q3_pm_accelerate_step.hpp"
#include "services/interfaces/workflow/quake3/q3_pm_types.hpp"
#include "services/interfaces/workflow/quake3/q3_pm_constants.hpp"
#include "services/interfaces/workflow/quake3/q3_wish_dir.hpp"
#include "services/interfaces/workflow/quake3/q3_slide_planes.hpp"
#include "services/interfaces/workflow_context.hpp"

#include <glm/glm.hpp>
#include <algorithm>
#include <cmath>
#include <utility>

namespace sdl3cpp::services::impl {


WorkflowQ3PmAccelerateStep::WorkflowQ3PmAccelerateStep(std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)) {}

std::string WorkflowQ3PmAccelerateStep::GetPluginId() const { return "q3.pm.accelerate"; }

void WorkflowQ3PmAccelerateStep::Execute(
    const WorkflowStepDefinition& /*step*/, WorkflowContext& context)
{
    auto* psPtr = context.TryGet<Q3PlayerState>("q3.ps");
    if (!psPtr) return;

    Q3PlayerState ps = *psPtr;

    const float dt          = static_cast<float>(context.GetDouble("frame.delta_time", 0.016));
    const float moveForward = context.Get<float>("input.move_forward", 0.f);
    const float moveRight   = context.Get<float>("input.move_right",   0.f);
    const float yaw         = context.Get<float>("q3.player_yaw",      0.f);

    const auto wish = q3::ComputeWish(moveForward, moveRight, yaw,
                                      q3::kMaxSpeed);
    if (wish.speed <= 0.f) {
        context.Set("q3.ps", ps);
        return;
    }
    // ioq3 PM_WalkMove projects the movement basis onto the ground plane
    // before accelerating, so on a slope "forward" means up the slope.
    // Without it the wish stays horizontal, the player pushes into the
    // surface instead of along it, and only the step machinery gets them
    // up — which is why walking up a slope used to stall where running
    // up the same slope did not.
    glm::vec3 wishDir = wish.direction;
    if (ps.onGround) {
        const glm::vec3 onPlane =
            q3::ClipVelocity(wishDir, ps.groundNormal, q3::kOverclip);
        const float length = glm::length(onPlane);
        if (length > 0.001f) {
            wishDir = onPlane / length;
        }
    }
    const float wishSpeed = wish.speed;

    const float accel        = ps.onGround ? q3::kAccelerate : q3::kAirAccelerate;
    const float currentSpeed = glm::dot(ps.velocity, wishDir);
    const float addSpeed     = wishSpeed - currentSpeed;

    if (addSpeed <= 0.f) {
        context.Set("q3.ps", ps);
        return;
    }

    const float accelSpeed = std::min(addSpeed, accel * wishSpeed * dt);
    ps.velocity += wishDir * accelSpeed;

    context.Set("q3.ps", ps);
}

}  // namespace sdl3cpp::services::impl
