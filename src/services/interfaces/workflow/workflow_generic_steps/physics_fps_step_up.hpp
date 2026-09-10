#pragma once

#include "services/interfaces/workflow/workflow_generic_steps/physics_fps_wish_velocity.hpp"

#include <btBulletDynamicsCommon.h>

namespace sdl3cpp::services::impl {

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

}  // namespace sdl3cpp::services::impl
