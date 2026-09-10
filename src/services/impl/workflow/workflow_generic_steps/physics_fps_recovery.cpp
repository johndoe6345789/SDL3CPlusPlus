#include "services/interfaces/workflow/workflow_generic_steps/physics_fps_recovery.hpp"

#include <algorithm>

namespace sdl3cpp::services::impl {

void UpdateFpsCrouchHeight(WorkflowContext& context, bool crouching,
                           float crouchHeight, float standHeight, float dt) {
    const float target  = crouching ? crouchHeight : standHeight;
    const float current = context.Get<float>("camera_eye_height", standHeight);
    constexpr float kLerpSpeedPerSecond = 8.0f;
    const float t = std::min(kLerpSpeedPerSecond * dt, 1.0f);
    context.Set<float>("camera_eye_height", current + (target - current) * t);
}

void ApplyFpsJamRecovery(btRigidBody* body, const btVector3& currentVelocity,
                         bool wantsMove, bool grounded, float moveSpeed,
                         float dt, float& jamTimeSeconds) {
    const float hVel =
        btVector3(currentVelocity.x(), 0.0f, currentVelocity.z()).length();
    if (!(wantsMove && grounded && hVel < moveSpeed * 0.2f)) {
        jamTimeSeconds = 0.0f;
        return;
    }

    jamTimeSeconds += dt;
    if (jamTimeSeconds <= 0.10f) {
        return;
    }
    btTransform xform;
    body->getMotionState()->getWorldTransform(xform);
    xform.setOrigin(xform.getOrigin() + btVector3(0, 0.10f, 0));
    body->setWorldTransform(xform);
    body->getMotionState()->setWorldTransform(xform);
    jamTimeSeconds = 0.0f;
}

bool ApplyFpsJump(btRigidBody* body, bool jumpKey, bool crouchKey,
                  bool grounded, bool wasJumping, float jumpVelocity) {
    if (jumpKey && !crouchKey && grounded && !wasJumping) {
        const btVector3 vel = body->getLinearVelocity();
        body->setLinearVelocity(btVector3(vel.x(), jumpVelocity, vel.z()));
        return true;
    }
    if (!jumpKey && grounded) {
        return false;
    }
    return wasJumping;
}

}  // namespace sdl3cpp::services::impl
