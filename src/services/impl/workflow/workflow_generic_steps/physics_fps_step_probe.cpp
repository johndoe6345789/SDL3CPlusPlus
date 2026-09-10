#include "services/interfaces/workflow/workflow_generic_steps/physics_fps_step_probe.hpp"

namespace sdl3cpp::services::impl {
namespace {

btCollisionWorld::ClosestRayResultCallback CastStepRay(
    btDiscreteDynamicsWorld* world, const btVector3& from,
    const btVector3& to) {
    btCollisionWorld::ClosestRayResultCallback hit(from, to);
    world->rayTest(from, to, hit);
    return hit;
}

}  // namespace

StepProbeResult ProbeFpsStep(btDiscreteDynamicsWorld* world,
                             const btVector3& origin, float feetY,
                             const btVector3& dir, float stepHeight,
                             float capsuleRadius, float capsuleHalfH) {
    StepProbeResult result;

    // Start probes just outside the capsule shell so they don't hit it.
    // probeReach is the lookahead: bigger means the snap-up detects stairs
    // sooner, before the capsule is fully wedged against a riser.
    const float probeStart = capsuleRadius + 0.05f;
    const float probeReach = 0.70f;

    // 1. Low probe: is something blocking us at shin height?
    const btVector3 lowFrom =
        origin + dir * probeStart -
        btVector3(0, capsuleHalfH + capsuleRadius - 0.10f, 0);
    const btVector3 lowTo = lowFrom + dir * probeReach;
    result.blocked        = CastStepRay(world, lowFrom, lowTo).hasHit();
    if (!result.blocked) {
        return result;
    }

    // 2. High probe: is the path clear at step_height?
    const btVector3 highFrom = lowFrom + btVector3(0, stepHeight + 0.05f, 0);
    const btVector3 highTo   = highFrom + dir * probeReach;
    result.pathClear         = !CastStepRay(world, highFrom, highTo).hasHit();
    if (!result.pathClear) {
        return result;
    }

    // 3. Down probe: find the top surface to step onto.
    const btVector3 downFrom = lowTo + btVector3(0, stepHeight, 0);
    const btVector3 downTo =
        btVector3(downFrom.x(), feetY - 0.05f, downFrom.z());
    const auto downHit = CastStepRay(world, downFrom, downTo);
    result.hasSurface  = downHit.hasHit();
    if (result.hasSurface) {
        result.surfaceY = downHit.m_hitPointWorld.y();
    }
    return result;
}

}  // namespace sdl3cpp::services::impl
