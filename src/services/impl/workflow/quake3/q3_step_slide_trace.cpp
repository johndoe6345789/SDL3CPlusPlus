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
    if (settleTrace.startSolid || settleTrace.fraction >= 1.f) {
        return false;
    }
    stepped.origin = settleTrace.endPos;

    // ioq3's PM_StepSlideMove takes the stepped position whenever the
    // settle lands on anything, and stops steep faces being climbed by
    // clipping the velocity against them (OVERCLIP) rather than by
    // throwing the step away. Requiring a walkable landing here instead
    // is what wedged the player on a stair's corner: the box overhangs
    // both treads, so Bullet reports the diagonal between them, which is
    // not walkable, and every frame discarded the step that would have
    // carried them up.
    const auto footing = services::impl::GroundProbe(
        world, settleTrace.endPos, kStepSize, stepped.mins, stepped.maxs, self);
    const bool walkable = footing.hit && footing.normal.y >= kMinWalkNormal;
    if (walkable && stepped.velocity.y < 0.f) {
        // Standing on the step: drop the fall so the player does not sink
        // straight back off it. Only the downward part comes out --
        // clipping against the reported normal is what launched the
        // player, since Bullet hands back the edge's diagonal and
        // horizontal speed came back as vertical.
        stepped.velocity.y = 0.f;
    }
    // Landing somewhere unwalkable keeps the position but leaves gravity
    // alone, so the player settles off it next frame. That is what keeps
    // a steep face from being climbed a step per frame without having to
    // discard the move.
    return true;
}

}  // namespace sdl3cpp::q3::step_slide_detail
