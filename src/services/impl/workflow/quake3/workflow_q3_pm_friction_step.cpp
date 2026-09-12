#include "services/interfaces/workflow/quake3/workflow_q3_pm_friction_step.hpp"
#include "services/interfaces/workflow/quake3/q3_pm_types.hpp"
#include "services/interfaces/workflow/quake3/q3_pm_tuning.hpp"
#include "services/interfaces/workflow/quake3/q3_pmove.hpp"
#include "services/interfaces/workflow_context.hpp"

#include <utility>

namespace sdl3cpp::services::impl {

WorkflowQ3PmFrictionStep::WorkflowQ3PmFrictionStep(
    std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)) {}

std::string WorkflowQ3PmFrictionStep::GetPluginId() const {
    return "q3.pm.friction";
}

void WorkflowQ3PmFrictionStep::Execute(const WorkflowStepDefinition& step,
                                       WorkflowContext& context) {
    auto* psPtr = context.TryGet<Q3PlayerState>("q3.ps");
    if (!psPtr) return;

    Q3PlayerState ps = *psPtr;
    q3::PmFriction(
        ps, static_cast<float>(context.GetDouble("frame.delta_time", 0.016)),
        Q3TuningOf(step));
    context.Set("q3.ps", ps);
}

}  // namespace sdl3cpp::services::impl
