#include "services/interfaces/workflow/quake3/workflow_q3_pm_accelerate_step.hpp"
#include "services/interfaces/workflow/quake3/q3_pm_types.hpp"
#include "services/interfaces/workflow_context.hpp"

#include <glm/glm.hpp>
#include <algorithm>
#include <cmath>
#include <utility>

namespace sdl3cpp::services::impl {

static constexpr float kMaxSpeedGround = 7.5f;   // scaled Q3 ground max speed
static constexpr float kMaxSpeedAir    = 6.0f;   // scaled Q3 air max speed
static constexpr float kAccelGround    = 50.0f;  // ground acceleration
static constexpr float kAccelAir       = 2.0f;   // air acceleration

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

    // Build wish direction in XZ plane, rotated by player yaw
    // yaw is the angle from +Z toward +X (standard Q3 / OpenGL convention)
    const float sinY = std::sin(yaw);
    const float cosY = std::cos(yaw);

    // Forward vector: -sinY, 0, -cosY  (same convention as fps_move_step)
    // Right   vector:  cosY, 0, -sinY
    glm::vec3 wishDir{
        -sinY * moveForward + cosY * moveRight,
        0.f,
        -cosY * moveForward - sinY * moveRight
    };

    const float wishLen = std::sqrt(wishDir.x * wishDir.x + wishDir.z * wishDir.z);
    if (wishLen < 0.001f) {
        context.Set("q3.ps", ps);
        return;
    }

    const float maxSpeed   = ps.onGround ? kMaxSpeedGround : kMaxSpeedAir;
    const float wishSpeed  = wishLen * maxSpeed;
    wishDir /= wishLen;   // normalize

    const float accel        = ps.onGround ? kAccelGround : kAccelAir;
    const float currentSpeed = ps.velocity.x * wishDir.x + ps.velocity.z * wishDir.z;
    const float addSpeed     = wishSpeed - currentSpeed;

    if (addSpeed <= 0.f) {
        context.Set("q3.ps", ps);
        return;
    }

    const float accelSpeed = std::min(addSpeed, accel * wishSpeed * dt);
    ps.velocity.x += accelSpeed * wishDir.x;
    ps.velocity.z += accelSpeed * wishDir.z;

    context.Set("q3.ps", ps);
}

}  // namespace sdl3cpp::services::impl
