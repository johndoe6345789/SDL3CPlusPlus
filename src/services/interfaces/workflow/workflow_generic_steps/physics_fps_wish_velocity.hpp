#pragma once

#include "services/interfaces/workflow/workflow_generic_steps/physics_fps_input.hpp"

#include <btBulletDynamicsCommon.h>

namespace sdl3cpp::services::impl {

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

}  // namespace sdl3cpp::services::impl
