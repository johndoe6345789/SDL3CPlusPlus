#pragma once

#include "services/interfaces/workflow/workflow_generic_steps/physics_body_params.hpp"

#include <btBulletDynamicsCommon.h>
#include <nlohmann/json.hpp>

namespace sdl3cpp::services::impl {

/// A newly created rigid body, its collision shape, and the visual
/// metadata JSON a renderer step derives its mesh transform from.
struct PhysicsBody {
    btRigidBody* body       = nullptr;
    btCollisionShape* shape = nullptr;
    nlohmann::json visual;
};

/**
 * @brief Builds a collision shape + rigid body from `params` and adds it
 *        to `world`.
 *
 * Uses a capsule shape when `params.shape == "capsule"`, otherwise a box.
 * When `params.lock_rotation > 0.5`, freezes angular motion (for
 * player/character bodies); mass == 0 bodies get full friction instead
 * (static geometry doesn't need deactivation management).
 */
PhysicsBody BuildPhysicsBody(btDiscreteDynamicsWorld* world,
                             const PhysicsBodyParams& params);

}  // namespace sdl3cpp::services::impl
