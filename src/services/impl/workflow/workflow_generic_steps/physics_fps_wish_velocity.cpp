#include "services/interfaces/workflow/workflow_generic_steps/physics_fps_wish_velocity.hpp"

#include <cmath>

namespace sdl3cpp::services::impl {

FpsWishVelocity ComputeFpsWishVelocity(const FpsMoveKeys& keys, float yaw,
                                       float moveSpeed, float sprintMultiplier,
                                       float crouchMultiplier) {
    const float sinY     = std::sin(yaw);
    const float cosY     = std::cos(yaw);
    const float forwardX = -sinY, forwardZ = -cosY;
    const float rightX = cosY, rightZ = -sinY;

    float moveX = 0.0f, moveZ = 0.0f;
    if (keys.forward) {
        moveX += forwardX;
        moveZ += forwardZ;
    }
    if (keys.back) {
        moveX -= forwardX;
        moveZ -= forwardZ;
    }
    if (keys.left) {
        moveX -= rightX;
        moveZ -= rightZ;
    }
    if (keys.right) {
        moveX += rightX;
        moveZ += rightZ;
    }

    float speed = moveSpeed;
    if (keys.crouch) {
        speed *= crouchMultiplier;
    } else if (keys.sprint) {
        speed *= sprintMultiplier;
    }

    FpsWishVelocity wish;
    wish.inputMagnitude = std::sqrt(moveX * moveX + moveZ * moveZ);
    if (wish.inputMagnitude > 0.001f) {
        wish.x = (moveX / wish.inputMagnitude) * speed;
        wish.z = (moveZ / wish.inputMagnitude) * speed;
    }
    return wish;
}

bool IsFpsBodyGrounded(btDiscreteDynamicsWorld* world, btRigidBody* body) {
    if (!world) {
        return false;
    }
    btTransform bodyTransform;
    body->getMotionState()->getWorldTransform(bodyTransform);
    const btVector3 from = bodyTransform.getOrigin();
    const btVector3 to   = from + btVector3(0, -1.2f, 0);
    btCollisionWorld::ClosestRayResultCallback rayResult(from, to);
    world->rayTest(from, to, rayResult);
    return rayResult.hasHit();
}

}  // namespace sdl3cpp::services::impl
