#include "services/interfaces/workflow/quake3/workflow_q3_pm_accelerate_step.hpp"
#include "services/interfaces/workflow/quake3/q3_pm_types.hpp"
#include "services/interfaces/workflow/quake3/q3_pm_tuning.hpp"
#include "services/interfaces/workflow/quake3/q3_pmove.hpp"
#include "services/interfaces/workflow_context.hpp"

#include <utility>

namespace sdl3cpp::services::impl {

WorkflowQ3PmAccelerateStep::WorkflowQ3PmAccelerateStep(
    std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)) {}

std::string WorkflowQ3PmAccelerateStep::GetPluginId() const {
    return "q3.pm.accelerate";
}

void WorkflowQ3PmAccelerateStep::Execute(const WorkflowStepDefinition& step,
                                         WorkflowContext& context) {
    auto* psPtr = context.TryGet<Q3PlayerState>("q3.ps");
    if (!psPtr) return;

    Q3PlayerState ps = *psPtr;
    q3::Q3UserCmd cmd;
    cmd.forwardMove = context.Get<float>("input.move_forward", 0.0f);
    cmd.rightMove   = context.Get<float>("input.move_right", 0.0f);
    cmd.yaw         = context.Get<float>("q3.player_yaw", 0.0f);
    cmd.sprint      = context.GetBool("input.sprint", false);

    q3::PmAccelerate(
        ps, cmd,
        static_cast<float>(context.GetDouble("frame.delta_time", 0.016)),
        Q3TuningOf(step));
    context.Set("q3.ps", ps);
}

}  // namespace sdl3cpp::services::impl
