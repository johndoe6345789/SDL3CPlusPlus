#include "services/interfaces/workflow/quake3/workflow_q3_pm_jump_step.hpp"
#include "services/interfaces/workflow/quake3/q3_pm_types.hpp"
#include "services/interfaces/workflow_context.hpp"

#include <glm/glm.hpp>
#include <utility>

namespace sdl3cpp::services::impl {

// 270 q3units/s * 0.03125 world/q3unit = 8.4375 ≈ 8.5
static constexpr float kJumpVelocity = 8.5f;

WorkflowQ3PmJumpStep::WorkflowQ3PmJumpStep(std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)) {}

std::string WorkflowQ3PmJumpStep::GetPluginId() const { return "q3.pm.jump"; }

void WorkflowQ3PmJumpStep::Execute(
    const WorkflowStepDefinition& /*step*/, WorkflowContext& context)
{
    auto* psPtr = context.TryGet<Q3PlayerState>("q3.ps");
    if (!psPtr) return;

    Q3PlayerState ps = *psPtr;

    const bool jumpPressed = context.GetBool("input.jump",      false);
    const bool jumpHeld    = context.GetBool("q3.pm_jump_held", false);

    if (jumpPressed && !jumpHeld && ps.onGround) {
        ps.velocity.y = kJumpVelocity;
        ps.onGround   = false;
    }

    // Track held state to prevent auto-bunnyhop
    context.Set("q3.pm_jump_held", jumpPressed);
    context.Set("q3.ps", ps);
}

}  // namespace sdl3cpp::services::impl
