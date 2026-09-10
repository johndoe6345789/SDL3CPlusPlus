#include "services/interfaces/workflow/workflow_generic_steps/physics_fps_velocity_integration.hpp"

#include <algorithm>
#include <cmath>

namespace sdl3cpp::services::impl {

void ApplyFpsMovementVelocity(btRigidBody* body, const FpsWishVelocity& wish,
                              bool grounded, float groundAccel,
                              float groundFriction, float airControl,
                              float dt) {
    const btVector3 currentVel = body->getLinearVelocity();

    if (grounded) {
        // Inertia model: accelerate horizontal velocity toward the wish
        // direction instead of snap-setting it; friction decelerates to zero
        // once input is released.
        float horizX = currentVel.x();
        float horizZ = currentVel.z();
        if (wish.inputMagnitude > 0.001f) {
            float diffX         = wish.x - horizX;
            float diffZ         = wish.z - horizZ;
            const float diffLen = std::sqrt(diffX * diffX + diffZ * diffZ);
            const float maxStep = groundAccel * dt;
            if (diffLen > maxStep && diffLen > 0.0f) {
                const float k = maxStep / diffLen;
                diffX *= k;
                diffZ *= k;
            }
            horizX += diffX;
            horizZ += diffZ;
        } else {
            const float curSpeed = std::sqrt(horizX * horizX + horizZ * horizZ);
            if (curSpeed > 0.001f) {
                const float drop = std::min(curSpeed, groundFriction * dt);
                const float k    = (curSpeed - drop) / curSpeed;
                horizX *= k;
                horizZ *= k;
            }
        }
        body->setLinearVelocity(btVector3(horizX, currentVel.y(), horizZ));
    } else {
        // Air control: blend input with current horizontal velocity.
        const float newX =
            currentVel.x() + (wish.x - currentVel.x()) * airControl;
        const float newZ =
            currentVel.z() + (wish.z - currentVel.z()) * airControl;
        body->setLinearVelocity(btVector3(newX, currentVel.y(), newZ));
    }
}

void ApplyFpsGravityScale(btRigidBody* body, bool grounded, float gravityScale,
                          float dt) {
    if (grounded || gravityScale == 1.0f) {
        return;
    }
    const float gravImpulse =
        -9.81f * body->getMass() * (gravityScale - 1.0f) * dt;
    body->applyCentralImpulse(btVector3(0, gravImpulse, 0));
}

}  // namespace sdl3cpp::services::impl
