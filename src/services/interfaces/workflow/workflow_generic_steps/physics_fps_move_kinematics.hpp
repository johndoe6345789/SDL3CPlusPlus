#pragma once

#include "services/interfaces/workflow_context.hpp"
#include "services/interfaces/workflow_step_definition.hpp"

#include <btBulletDynamicsCommon.h>

namespace sdl3cpp::services::impl {

/// Held movement keys for one frame, read from input.poll's context keys.
struct FpsMoveKeys {
    bool forward = false, back = false, left = false, right = false;
    bool jump = false, sprint = false, crouch = false;
};

/// Reads input_key_{w,a,s,d,space,shift,ctrl} (set by input.poll).
FpsMoveKeys ReadFpsMoveKeys(const WorkflowContext& context);

/// The step's tunable parameters, each with the original plugin's default.
struct FpsMoveParams {
    float moveSpeed        = 6.0f;
    float sprintMultiplier = 1.8f;
    float crouchMultiplier = 0.4f;
    float crouchHeight     = 0.8f;
    float standHeight      = 1.6f;
    float airControl       = 0.3f;
    float gravityScale     = 1.0f;
    float groundAccel      = 35.0f;
    float groundFriction   = 30.0f;
    float stepHeight       = 0.55f;
    float jumpVelocity     = 6.5f;
};

FpsMoveParams ReadFpsMoveParams(const WorkflowStepDefinition& step);

/// Lerps the camera's eye height toward the crouch/stand target.
void UpdateFpsCrouchHeight(WorkflowContext& context, bool crouching,
                           float crouchHeight, float standHeight, float dt);

/// Horizontal wish direction and speed, already yaw-relative and normalised.
struct FpsWishVelocity {
    float x = 0.0f, z = 0.0f;
    /// Magnitude before normalisation; > 0.001 means "player wants to move".
    float inputMagnitude = 0.0f;
};

/// @param yaw Camera yaw in radians (camera.fps.update's `camera_yaw`).
FpsWishVelocity ComputeFpsWishVelocity(const FpsMoveKeys& keys, float yaw,
                                       float moveSpeed, float sprintMultiplier,
                                       float crouchMultiplier);

/// Casts a short ray beneath the body to test for ground contact.
bool IsFpsBodyGrounded(btDiscreteDynamicsWorld* world, btRigidBody* body);

/**
 * @brief Advances horizontal velocity toward the wish direction.
 *
 * Grounded: accelerates/decelerates at groundAccel/groundFriction units per
 * second (CalcFriction-style inertia). Airborne: blends toward the wish
 * direction by airControl per frame (Quake-style air control).
 */
void ApplyFpsMovementVelocity(btRigidBody* body, const FpsWishVelocity& wish,
                              bool grounded, float groundAccel,
                              float groundFriction, float airControl, float dt);

/**
 * @brief Applies a symmetric per-player gravity multiplier while airborne.
 *
 * Implemented as an impulse (force * dt) rather than applyCentralForce so the
 * integrated effect matches regardless of framerate — Bullet accumulates
 * forces across substeps, so a per-frame force call at 240Hz would otherwise
 * multiply 4x versus 60Hz.
 */
void ApplyFpsGravityScale(btRigidBody* body, bool grounded, float gravityScale,
                          float dt);

/**
 * @brief Steps the body up a low obstacle (e.g. Q3 stairs) it is walking into.
 *
 * Capsule rigid bodies can't roll over stairs taller than their radius, so
 * this runs three raycasts each rate-limited tick (Q3's PM_StepSlideMove
 * pattern, simplified) to detect a walkable ledge and teleport the body onto
 * it. `accumulatorSeconds` rate-limits this to ~60Hz across all framerates —
 * without it, high-refresh frames would multi-teleport up a single step.
 */
void TryFpsStepUp(btRigidBody* body, btDiscreteDynamicsWorld* world,
                  const FpsWishVelocity& wish, float stepHeight, float dt,
                  float& accumulatorSeconds);

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
