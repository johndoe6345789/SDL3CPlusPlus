#include "services/interfaces/workflow/workflow_generic_steps/physics_fps_step_up.hpp"
#include "services/interfaces/workflow/workflow_generic_steps/physics_fps_step_probe.hpp"

namespace sdl3cpp::services::impl {

void TryFpsStepUp(btRigidBody* body, btDiscreteDynamicsWorld* world,
                  const FpsWishVelocity& wish, float stepHeight, float dt,
                  float& accumulatorSeconds) {
    constexpr float kStepInterval = 1.0f / 60.0f;
    accumulatorSeconds += dt;
    const bool stepReady = accumulatorSeconds >= kStepInterval;

    btVector3 dir(wish.x, 0.0f, wish.z);
    const float dirMag = dir.length();
    if (!stepReady || !world || dirMag <= 0.0001f || stepHeight <= 0.0f) {
        return;
    }
    accumulatorSeconds = 0.0f;
    dir /= dirMag;

    const float capsuleRadius = 0.3f;  // matches q3_game.json player shape
    const float capsuleHalfH  = 0.5f;  // height/2
    btTransform xform;
    body->getMotionState()->getWorldTransform(xform);
    const btVector3 origin = xform.getOrigin();
    const float feetY      = origin.y() - capsuleHalfH - capsuleRadius;

    const StepProbeResult probe =
        ProbeFpsStep(world, origin, feetY, dir, stepHeight, capsuleRadius,
                    capsuleHalfH);
    if (!probe.blocked || !probe.pathClear || !probe.hasSurface) {
        return;
    }

    const float deltaY = probe.surfaceY - feetY;
    if (deltaY <= 0.05f || deltaY >= stepHeight) {
        return;
    }

    // Snap up AND forward. The forward nudge places the capsule fully onto
    // the new tread so it does not immediately snag the next riser; without
    // it, each step costs a frame of solver-fighting and feels
    // staircase-shaped instead of ramp-shaped. Horizontal velocity is left
    // alone so movement up the staircase stays smooth.
    const float forwardNudge = capsuleRadius + 0.05f;
    xform.setOrigin(origin + btVector3(0, deltaY + 0.02f, 0) +
                    dir * forwardNudge);
    body->setWorldTransform(xform);
    body->getMotionState()->setWorldTransform(xform);
}

}  // namespace sdl3cpp::services::impl
