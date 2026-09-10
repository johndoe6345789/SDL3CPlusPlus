#pragma once

#include "services/interfaces/workflow/workflow_generic_steps/physics_fps_wish_velocity.hpp"

#include <btBulletDynamicsCommon.h>

namespace sdl3cpp::services::impl {

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

}  // namespace sdl3cpp::services::impl
