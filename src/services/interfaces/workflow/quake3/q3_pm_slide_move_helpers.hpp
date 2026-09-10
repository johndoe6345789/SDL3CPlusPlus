#pragma once

#include "services/interfaces/workflow/quake3/q3_pm_types.hpp"
#include "services/interfaces/workflow_context.hpp"

#include <btBulletDynamicsCommon.h>
#include <glm/glm.hpp>

namespace sdl3cpp::services::impl {

/// v = v - dot(v, n) * overbounce * n. overbounce > 1 prevents the player
/// from sticking to surfaces. Currently unused by the slide-move loop
/// itself (q3::ResolveAgainstPlanes does the clipping), kept as ioq3's
/// PM_ClipVelocity reference for any future caller.
glm::vec3 ClipVelocity(const glm::vec3& v, const glm::vec3& normal,
                       float overbounce);

/**
 * @brief Runs one frame of Q3's iterative slide-move (bg_slidemove.c
 * PM_SlideMove) against `world`, mutating `ps.origin`/`ps.velocity`.
 *
 * Up to 4 collision bumps: sweep a box from origin along velocity *
 * timeLeft, advance to the hit point, and make the velocity parallel to
 * every plane hit so far (a genuine three-plane corner stops the player
 * dead). Re-hitting an already-recorded plane nudges velocity out along
 * its normal and retries rather than recording a duplicate, matching
 * ioq3's behavior.
 */
void RunQ3SlideMove(Q3PlayerState& ps, btDiscreteDynamicsWorld* world,
                    const WorkflowContext& context, float dt);

}  // namespace sdl3cpp::services::impl
