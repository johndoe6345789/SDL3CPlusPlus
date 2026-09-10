#pragma once

#include "services/interfaces/workflow_step_definition.hpp"

#include <btBulletDynamicsCommon.h>
#include <nlohmann/json.hpp>

#include <string>

namespace sdl3cpp::services::impl {

/// Parameters for one `physics.body.add` call, already resolved from the
/// step's JSON parameters.
struct PhysicsBodyParams {
    std::string name  = "body";
    std::string shape = "box";
    float mass        = 0.0f;
    float pos_x = 0.0f, pos_y = 0.0f, pos_z = 0.0f;
    // Box dimensions
    float size_x = 1.0f, size_y = 1.0f, size_z = 1.0f;
    // Capsule dimensions
    float radius = 0.4f, height = 1.2f;
    // Flags
    float lock_rotation = 0.0f;
    float is_player     = 0.0f;
    float spinning      = 0.0f;
    float spin_speed_x = 1.0f, spin_speed_y = 0.7f;
    float visible = 1.0f;
};

/// A newly created rigid body, its collision shape, and the visual
/// metadata JSON a renderer step derives its mesh transform from.
struct PhysicsBody {
    btRigidBody* body       = nullptr;
    btCollisionShape* shape = nullptr;
    nlohmann::json visual;
};

/// Reads every PhysicsBodyParams field from `step`'s JSON parameters,
/// falling back to PhysicsBodyParams' defaults for any not present.
PhysicsBodyParams ResolvePhysicsBodyParams(const WorkflowStepDefinition& step);

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
