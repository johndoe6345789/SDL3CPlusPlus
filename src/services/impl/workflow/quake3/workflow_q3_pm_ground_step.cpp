#include "services/interfaces/workflow/quake3/workflow_q3_pm_ground_step.hpp"
#include "services/interfaces/workflow/quake3/q3_pm_types.hpp"
#include "services/interfaces/workflow/quake3/q3_pmove.hpp"
#include "services/interfaces/workflow_context.hpp"

#include <btBulletDynamicsCommon.h>
#include <utility>

namespace sdl3cpp::services::impl {

WorkflowQ3PmGroundStep::WorkflowQ3PmGroundStep(std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)) {}

std::string WorkflowQ3PmGroundStep::GetPluginId() const {
    return "q3.pm.ground";
}

void WorkflowQ3PmGroundStep::Execute(const WorkflowStepDefinition& /*step*/,
                                     WorkflowContext& context) {
    auto* psPtr = context.TryGet<Q3PlayerState>("q3.ps");
    if (!psPtr) return;

    Q3PlayerState ps = *psPtr;
    q3::PmGroundTrace(
        ps, context.Get<btDiscreteDynamicsWorld*>("physics_world", nullptr),
        static_cast<float>(context.GetDouble("frame.delta_time", 0.016)),
        PlayerBody(context));
    context.Set("q3.ps", ps);
}

}  // namespace sdl3cpp::services::impl
