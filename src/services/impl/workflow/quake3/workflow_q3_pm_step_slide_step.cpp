#include "services/interfaces/workflow/quake3/workflow_q3_pm_step_slide_step.hpp"
#include "services/interfaces/workflow/quake3/q3_step_slide.hpp"
#include "services/interfaces/workflow/quake3/q3_pm_types.hpp"
#include "services/interfaces/workflow_context.hpp"

#include <btBulletDynamicsCommon.h>

namespace sdl3cpp::services::impl {

WorkflowQ3PmStepSlideStep::WorkflowQ3PmStepSlideStep(
    std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)) {}

std::string WorkflowQ3PmStepSlideStep::GetPluginId() const {
    return "q3.pm.step_slide";
}

void WorkflowQ3PmStepSlideStep::Execute(const WorkflowStepDefinition&,
                                        WorkflowContext& context) {
    auto* psPtr = context.TryGet<Q3PlayerState>("q3.ps");
    if (!psPtr) return;

    Q3PlayerState ps = *psPtr;
    auto* world =
        context.Get<btDiscreteDynamicsWorld*>("physics_world", nullptr);
    const float dt =
        static_cast<float>(context.GetDouble("frame.delta_time", 0.016));

    if (!world) {
        ps.origin += ps.velocity * dt;
        context.Set("q3.ps", ps);
        context.Set("q3.player_pos", ps.origin);
        return;
    }

    const btCollisionObject* self = PlayerBody(context);
    const auto stepDelta = q3::ApplyQ3StepSlideMove(ps, world, dt, self);

    // A rejected step attempt (nullopt) leaves q3.step_delta untouched,
    // matching the original monolithic step's early returns.
    if (stepDelta) {
        context.Set<float>("q3.step_delta", *stepDelta);
    }

    context.Set("q3.ps", ps);
    context.Set("q3.player_pos", ps.origin);
}

}  // namespace sdl3cpp::services::impl
