#include "services/interfaces/workflow/quake3/workflow_q3_pm_slide_move_step.hpp"
#include "services/interfaces/workflow/quake3/q3_pm_slide_move_helpers.hpp"
#include "services/interfaces/workflow/quake3/q3_pm_types.hpp"
#include "services/interfaces/workflow_context.hpp"

#include <btBulletDynamicsCommon.h>
#include <glm/glm.hpp>

namespace sdl3cpp::services::impl {

WorkflowQ3PmSlideMoveStep::WorkflowQ3PmSlideMoveStep(
    std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)) {}

std::string WorkflowQ3PmSlideMoveStep::GetPluginId() const {
    return "q3.pm.slide_move";
}

void WorkflowQ3PmSlideMoveStep::Execute(const WorkflowStepDefinition& /*step*/,
                                        WorkflowContext& context) {
    auto* psPtr = context.TryGet<Q3PlayerState>("q3.ps");
    if (!psPtr) return;

    Q3PlayerState ps = *psPtr;
    auto* world =
        context.Get<btDiscreteDynamicsWorld*>("physics_world", nullptr);
    const float dt =
        static_cast<float>(context.GetDouble("frame.delta_time", 0.016));

    if (!world) {
        // No physics world: integrate velocity directly.
        ps.origin += ps.velocity * dt;
        context.Set("q3.ps", ps);
        context.Set("q3.player_pos", ps.origin);
        return;
    }

    RunQ3SlideMove(ps, world, context, dt);

    context.Set("q3.ps", ps);
    context.Set("q3.player_pos", ps.origin);
    // Yaw is managed by the camera step; just re-publish what is already
    // set (a no-op write to keep the key alive for any downstream reader).
    context.Set("q3.player_yaw", context.Get<float>("q3.player_yaw", 0.f));
}

}  // namespace sdl3cpp::services::impl
