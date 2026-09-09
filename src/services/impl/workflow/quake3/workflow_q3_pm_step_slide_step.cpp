#include "services/interfaces/workflow/quake3/workflow_q3_pm_step_slide_step.hpp"
#include "services/interfaces/workflow/quake3/q3_slide_move.hpp"
#include "services/interfaces/workflow/quake3/q3_slide_planes.hpp"
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
    auto* world = context.Get<btDiscreteDynamicsWorld*>("physics_world",
                                                        nullptr);
    const float dt =
        static_cast<float>(context.GetDouble("frame.delta_time", 0.016));

    if (!world) {
        ps.origin += ps.velocity * dt;
        context.Set("q3.ps", ps);
        context.Set("q3.player_pos", ps.origin);
        return;
    }

    const glm::vec3 startOrigin = ps.origin;
    const glm::vec3 startVelocity = ps.velocity;

    // Stepping is for walking into something. Falling onto the floor
    // also reports the move as obstructed, and attempting a step there
    // lifted the player a little every frame until they floated away
    // from the map entirely.
    const glm::vec3 horizontal(startVelocity.x, 0.f, startVelocity.z);
    const bool movingHorizontally =
        glm::dot(horizontal, horizontal) > 0.01f;

    if (!q3::SlideMove(ps, world, dt) || !movingHorizontally) {
        context.Set("q3.ps", ps);
        context.Set("q3.player_pos", ps.origin);
        context.Set<float>("q3.step_delta", 0.f);
        return;  // reached the target first try, nothing to step over
    }

    // Never step while still rising, unless there is ground below.
    const glm::vec3 down = startOrigin - glm::vec3(0.f, q3::kStepSize, 0.f);
    const auto downTrace = TraceBox(world, startOrigin, down, ps.mins, ps.maxs);
    if (startVelocity.y > 0.f &&
        (downTrace.fraction == 1.f || downTrace.normal.y < 0.7f)) {
        context.Set("q3.ps", ps);
        context.Set("q3.player_pos", ps.origin);
        return;
    }

    const glm::vec3 up = startOrigin + glm::vec3(0.f, q3::kStepSize, 0.f);
    const auto upTrace = TraceBox(world, startOrigin, up, ps.mins, ps.maxs);
    if (upTrace.startSolid) {
        context.Set("q3.ps", ps);
        context.Set("q3.player_pos", ps.origin);
        return;  // no headroom to step into
    }

    const glm::vec3 slidResult = ps.origin;
    const float stepSize = upTrace.endPos.y - startOrigin.y;

    Q3PlayerState stepped = ps;
    stepped.origin = upTrace.endPos;
    stepped.velocity = startVelocity;
    q3::SlideMove(stepped, world, dt);

    // Settle back down onto whatever was stepped onto.
    const glm::vec3 settle =
        stepped.origin - glm::vec3(0.f, stepSize, 0.f);
    const auto settleTrace =
        TraceBox(world, stepped.origin, settle, stepped.mins, stepped.maxs);
    // The step is only real if there is something to stand on within a
    // step height. A settle trace that reaches the bottom found nothing,
    // and a trace that cannot start found nothing knowable: in both
    // cases keeping the raised origin lets the player ratchet up a flat
    // wall a step per frame, which is exactly what happened.
    if (settleTrace.startSolid || settleTrace.fraction >= 1.f) {
        // Could not settle back down, so we have no idea what is under
        // the player. Keeping the raised origin here is what let the
        // player ratchet up a flat wall a step per frame; discard the
        // attempt and use the plain slide instead.
        context.Set("q3.ps", ps);
        context.Set("q3.player_pos", ps.origin);
        return;
    }
    stepped.origin = settleTrace.endPos;
    if (settleTrace.fraction < 1.f) {
        stepped.velocity = q3::ClipVelocity(
            stepped.velocity, settleTrace.normal, q3::kOverclip);
    }

    // ioq3 takes the stepped move; the guard above is what keeps it
    // honest. Record the rise so a step sound can be chosen later.
    (void)slidResult;
    ps = stepped;
    context.Set<float>("q3.step_delta", ps.origin.y - startOrigin.y);

    context.Set("q3.ps", ps);
    context.Set("q3.player_pos", ps.origin);
}

}  // namespace sdl3cpp::services::impl
