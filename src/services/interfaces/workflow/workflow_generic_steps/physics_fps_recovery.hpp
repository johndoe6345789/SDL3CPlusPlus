#pragma once

#include "services/interfaces/workflow_context.hpp"

#include <btBulletDynamicsCommon.h>

namespace sdl3cpp::services::impl {

/// Lerps the camera's eye height toward the crouch/stand target.
void UpdateFpsCrouchHeight(WorkflowContext& context, bool crouching,
                           float crouchHeight, float standHeight, float dt);

/**
 * @brief Nudges the body up if movement input isn't producing real velocity.
 *
 * Catches the case where the step-up probe missed and the player is wedged
 * against a riser: after 100ms of near-zero realized horizontal velocity
 * despite wanting to move, lifts the capsule clear.
 */
void ApplyFpsJamRecovery(btRigidBody* body, const btVector3& currentVelocity,
                         bool wantsMove, bool grounded, float moveSpeed,
                         float dt, float& jamTimeSeconds);

/**
 * @brief Quake-style impulse jump: sets upward velocity once on press.
 *
 * Height comes from gravity afterward, not a held key; `wasJumping` (the
 * context's latched `player_jumping` flag) prevents auto-bunnyhop.
 * @return the new value for `player_jumping`.
 */
bool ApplyFpsJump(btRigidBody* body, bool jumpKey, bool crouchKey,
                  bool grounded, bool wasJumping, float jumpVelocity);

}  // namespace sdl3cpp::services::impl
