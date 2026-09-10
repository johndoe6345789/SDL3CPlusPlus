#include "services/interfaces/workflow/quake3/q3_step_slide_internal.hpp"
#include "services/interfaces/workflow/quake3/q3_slide_move.hpp"
#include "services/interfaces/workflow/quake3/q3_slide_planes.hpp"

#include <btBulletDynamicsCommon.h>

namespace sdl3cpp::q3::step_slide_detail {

using services::impl::Q3PlayerState;
using services::impl::Q3Trace;

std::optional<Q3Trace> ComputeStepUpTrace(btDiscreteDynamicsWorld* world,
                                          const glm::vec3& startOrigin,
                                          const glm::vec3& startVelocity,
                                          const Q3PlayerState& ps,
                                          const btCollisionObject* self) {
    // Never step while still rising, unless there is ground below.
    const auto downTrace = services::impl::GroundProbe(
        world, startOrigin, kStepSize, ps.mins, ps.maxs, self);
    if (startVelocity.y > 0.f &&
        (downTrace.fraction == 1.f || downTrace.normal.y < kMinWalkNormal)) {
        return std::nullopt;
    }

    const glm::vec3 up = startOrigin + glm::vec3(0.f, kStepSize, 0.f);
    const auto upTrace = services::impl::TraceBox(world, startOrigin, up,
                                                  ps.mins, ps.maxs, self);
    if (upTrace.startSolid) {
        return std::nullopt;  // no headroom to step into
    }
    return upTrace;
}

bool SettleSteppedMove(btDiscreteDynamicsWorld* world, Q3PlayerState& stepped,
                       float stepSize, const btCollisionObject* self) {
    // Settle back down onto whatever was stepped onto.
    const glm::vec3 settle = stepped.origin - glm::vec3(0.f, stepSize, 0.f);
    const auto settleTrace = services::impl::TraceBox(
        world, stepped.origin, settle, stepped.mins, stepped.maxs, self);
    // The step is only real if there is something to stand on within a
    // step height. A settle trace that reaches the bottom found nothing,
    // and a trace that cannot start found nothing knowable: in both
    // cases keeping the raised origin lets the player ratchet up a flat
    // wall a step per frame, which is exactly what happened.
    // Only step onto something that could be stood on. Without this the
    // step machinery climbs any slope at all, one step per frame, because
    // the settle below zeroes the downward velocity that would otherwise
    // carry the player back off it.
    //
    // The settle trace positions the player; ask what is underfoot to
    // decide whether the step was legitimate.
    const auto footing = services::impl::GroundProbe(
        world, settleTrace.endPos, kStepSize, stepped.mins, stepped.maxs, self);
    const bool settledOnWalkable = settleTrace.fraction < 1.f && footing.hit &&
                                   footing.normal.y >= kMinWalkNormal;
    if (settleTrace.startSolid || !settledOnWalkable) {
        // Could not settle back down, so we have no idea what is under
        // the player. Keeping the raised origin here is what let the
        // player ratchet up a flat wall a step per frame; discard the
        // attempt and use the plain slide instead.
        return false;
    }
    stepped.origin = settleTrace.endPos;
    if (settleTrace.fraction < 1.f) {
        // The settle is a straight-down probe onto whatever was stepped
        // onto, so the only thing to take out of the velocity is its
        // downward part. Clipping against the reported normal here is
        // what launched the player: the box overhangs the step's edge,
        // Bullet reports the edge's diagonal, and horizontal speed came
        // back as vertical.
        if (stepped.velocity.y < 0.f) stepped.velocity.y = 0.f;
    }
    return true;
}

}  // namespace sdl3cpp::q3::step_slide_detail
