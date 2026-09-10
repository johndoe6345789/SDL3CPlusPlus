#pragma once

#include <btBulletDynamicsCommon.h>

namespace sdl3cpp::services::impl {

/// Result of the three-raycast stair probe used by TryFpsStepUp.
struct StepProbeResult {
    bool blocked    = false;  // Low probe: something at shin height.
    bool pathClear  = false;  // High probe: nothing at step_height.
    bool hasSurface = false;  // Down probe: found a walkable top surface.
    float surfaceY  = 0.0f;
};

/**
 * @brief Casts the low/high/down probe triplet for one step-up attempt.
 *
 * `origin`/`feetY` are the body's current transform; `dir` is the
 * normalised horizontal movement direction. Probe geometry (start offset,
 * reach) is tuned for the capsule dimensions passed in.
 */
StepProbeResult ProbeFpsStep(btDiscreteDynamicsWorld* world,
                             const btVector3& origin, float feetY,
                             const btVector3& dir, float stepHeight,
                             float capsuleRadius, float capsuleHalfH);

}  // namespace sdl3cpp::services::impl
