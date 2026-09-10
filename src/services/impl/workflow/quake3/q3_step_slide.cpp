#include "services/interfaces/workflow/quake3/q3_step_slide.hpp"
#include "services/interfaces/workflow/quake3/q3_slide_move.hpp"
#include "services/interfaces/workflow/quake3/q3_step_slide_internal.hpp"

#include <btBulletDynamicsCommon.h>

namespace sdl3cpp::q3 {

using services::impl::Q3PlayerState;

std::optional<float> ApplyQ3StepSlideMove(Q3PlayerState& ps,
                                          btDiscreteDynamicsWorld* world,
                                          float dt,
                                          const btCollisionObject* self) {
    namespace detail = step_slide_detail;

    const glm::vec3 startOrigin   = ps.origin;
    const glm::vec3 startVelocity = ps.velocity;

    // Stepping is for walking into something. Falling onto the floor
    // also reports the move as obstructed, and attempting a step there
    // lifted the player a little every frame until they floated away
    // from the map entirely.
    const glm::vec3 horizontal(startVelocity.x, 0.f, startVelocity.z);
    const bool movingHorizontally = glm::dot(horizontal, horizontal) > 0.01f;

    // Deliberately stricter than ioq3, which will also step while
    // airborne. Every airborne step attempt here has ended up letting
    // the player ratchet up a flat wall, and stepping exists to get up
    // stairs and ledges while walking, so it is gated on standing on
    // something.
    const bool canStep = ps.onGround && movingHorizontally;

    const bool blocked = SlideMove(ps, world, dt, self);
    if (!blocked || !canStep) {
        return 0.f;  // reached the target first try, nothing to step over
    }

    const auto upTrace =
        detail::ComputeStepUpTrace(world, startOrigin, startVelocity, ps, self);
    if (!upTrace) {
        return std::nullopt;
    }
    const float stepSize = upTrace->endPos.y - startOrigin.y;

    Q3PlayerState stepped = ps;
    stepped.origin        = upTrace->endPos;
    stepped.velocity      = startVelocity;
    SlideMove(stepped, world, dt, self);

    if (!detail::SettleSteppedMove(world, stepped, stepSize, self)) {
        return std::nullopt;
    }

    // ioq3 takes the stepped move; the guard above is what keeps it
    // honest. Record the rise so a step sound can be chosen later.
    ps = stepped;
    return ps.origin.y - startOrigin.y;
}

}  // namespace sdl3cpp::q3
