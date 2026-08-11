#include "services/interfaces/workflow/quake3/workflow_q3_pm_friction_step.hpp"
#include "services/interfaces/workflow/quake3/q3_pm_types.hpp"
#include "services/interfaces/workflow_context.hpp"

#include <glm/glm.hpp>
#include <algorithm>
#include <cmath>
#include <utility>

namespace sdl3cpp::services::impl {

static constexpr float kStopSpeed = 1.5f;   // pm_stopspeed (scaled)
static constexpr float kFriction  = 8.0f;   // pm_friction  (scaled)

WorkflowQ3PmFrictionStep::WorkflowQ3PmFrictionStep(std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)) {}

std::string WorkflowQ3PmFrictionStep::GetPluginId() const { return "q3.pm.friction"; }

void WorkflowQ3PmFrictionStep::Execute(
    const WorkflowStepDefinition& /*step*/, WorkflowContext& context)
{
    auto* psPtr = context.TryGet<Q3PlayerState>("q3.ps");
    if (!psPtr) return;

    Q3PlayerState ps = *psPtr;

    if (!ps.onGround) {
        context.Set("q3.ps", ps);
        return;
    }

    const float dt = static_cast<float>(context.GetDouble("frame.delta_time", 0.016));

    const float speed = std::sqrt(ps.velocity.x * ps.velocity.x +
                                  ps.velocity.z * ps.velocity.z);
    if (speed < 0.1f) {
        ps.velocity.x = 0.f;
        ps.velocity.z = 0.f;
        context.Set("q3.ps", ps);
        return;
    }

    const float control  = std::max(speed, kStopSpeed);
    const float drop     = control * kFriction * dt;
    const float newSpeed = std::max(0.f, speed - drop);

    const float scale = newSpeed / speed;
    ps.velocity.x *= scale;
    ps.velocity.z *= scale;

    context.Set("q3.ps", ps);
}

}  // namespace sdl3cpp::services::impl
