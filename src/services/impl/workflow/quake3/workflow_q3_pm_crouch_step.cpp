#include "services/interfaces/workflow/quake3/workflow_q3_pm_crouch_step.hpp"
#include "services/interfaces/workflow/quake3/q3_pm_types.hpp"
#include "services/interfaces/workflow_context.hpp"

#include <btBulletDynamicsCommon.h>
#include <glm/glm.hpp>
#include <utility>

namespace sdl3cpp::services::impl {

static constexpr float kStandMaxsY  = 1.4f;
static constexpr float kCrouchMaxsY = 0.85f;

WorkflowQ3PmCrouchStep::WorkflowQ3PmCrouchStep(std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)) {}

std::string WorkflowQ3PmCrouchStep::GetPluginId() const { return "q3.pm.crouch"; }

void WorkflowQ3PmCrouchStep::Execute(
    const WorkflowStepDefinition& /*step*/, WorkflowContext& context)
{
    auto* psPtr = context.TryGet<Q3PlayerState>("q3.ps");
    if (!psPtr) return;

    Q3PlayerState ps = *psPtr;

    const bool crouchPressed = context.GetBool("input.crouch", false);
    auto* world = context.Get<btDiscreteDynamicsWorld*>("physics_world", nullptr);

    if (crouchPressed && !ps.crouching) {
        // Transition: standing → crouching
        ps.crouching  = true;
        ps.maxs.y     = kCrouchMaxsY;
    } else if (!crouchPressed && ps.crouching) {
        // Transition: crouching → standing — only if there is room above
        bool canStand = true;
        if (world) {
            // Trace upward by the height difference to check for headroom
            const float rise = kStandMaxsY - kCrouchMaxsY;
            const glm::vec3 traceEnd = ps.origin + glm::vec3(0.f, rise, 0.f);
            Q3Trace tr = TraceBox(world, ps.origin, traceEnd, ps.mins, ps.maxs);
            canStand = (tr.fraction >= 1.f);
        }
        if (canStand) {
            ps.crouching = false;
            ps.maxs.y    = kStandMaxsY;
        }
    }

    context.Set("q3.ps", ps);

    // Backward-compat writes for HUD / bots
    context.Set("q3.player_pos",   ps.origin);
    context.Set("q3.player_yaw",   context.Get<float>("q3.player_yaw",   0.f));
    context.Set("q3.player_pitch", context.Get<float>("q3.player_pitch",  0.f));
}

}  // namespace sdl3cpp::services::impl
