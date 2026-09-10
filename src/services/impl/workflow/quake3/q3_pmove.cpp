#include "services/interfaces/workflow/quake3/q3_pmove.hpp"

#include "services/interfaces/workflow/quake3/q3_step_slide.hpp"

namespace sdl3cpp::q3 {

void Q3PmoveOne(services::impl::Q3PlayerState& ps, const Q3UserCmd& cmd,
                btDiscreteDynamicsWorld* world, float dt,
                const btCollisionObject* self) {
    PmGroundTrace(ps, world, dt, self);
    PmFriction(ps, dt);
    PmAccelerate(ps, cmd, dt);

    if (cmd.jump && ps.onGround) {
        ps.onGround   = false;
        ps.velocity.y = kJumpVelocity;
    }

    if (!world) {
        ps.origin += ps.velocity * dt;
        return;
    }
    ApplyQ3StepSlideMove(ps, world, dt, self);
}

}  // namespace sdl3cpp::q3
