#include "services/interfaces/workflow/workflow_generic_steps/physics_fps_move_kinematics.hpp"
#include "services/interfaces/workflow/workflow_step_parameter_resolver.hpp"

#include <algorithm>
#include <cmath>

namespace sdl3cpp::services::impl {
namespace {

float NumberParameter(const WorkflowStepParameterResolver& params,
                      const WorkflowStepDefinition& step, const char* key,
                      float fallback) {
    const auto* p       = params.FindParameter(step, key);
    const bool isNumber = p && p->type == WorkflowParameterValue::Type::Number;
    return isNumber ? static_cast<float>(p->numberValue) : fallback;
}

}  // namespace

FpsMoveKeys ReadFpsMoveKeys(const WorkflowContext& context) {
    FpsMoveKeys keys;
    keys.forward = context.GetBool("input_key_w", false);
    keys.left    = context.GetBool("input_key_a", false);
    keys.back    = context.GetBool("input_key_s", false);
    keys.right   = context.GetBool("input_key_d", false);
    keys.jump    = context.GetBool("input_key_space", false);
    keys.sprint  = context.GetBool("input_key_shift", false);
    keys.crouch  = context.GetBool("input_key_ctrl", false);
    return keys;
}

FpsMoveParams ReadFpsMoveParams(const WorkflowStepDefinition& step) {
    WorkflowStepParameterResolver params;
    FpsMoveParams p;
    p.moveSpeed = NumberParameter(params, step, "move_speed", p.moveSpeed);
    p.sprintMultiplier =
        NumberParameter(params, step, "sprint_multiplier", p.sprintMultiplier);
    p.crouchMultiplier =
        NumberParameter(params, step, "crouch_multiplier", p.crouchMultiplier);
    p.crouchHeight =
        NumberParameter(params, step, "crouch_height", p.crouchHeight);
    p.standHeight =
        NumberParameter(params, step, "stand_height", p.standHeight);
    p.airControl = NumberParameter(params, step, "air_control", p.airControl);
    p.gravityScale =
        NumberParameter(params, step, "gravity_scale", p.gravityScale);
    p.groundAccel =
        NumberParameter(params, step, "ground_accel", p.groundAccel);
    p.groundFriction =
        NumberParameter(params, step, "ground_friction", p.groundFriction);
    p.stepHeight = NumberParameter(params, step, "step_height", p.stepHeight);
    p.jumpVelocity =
        NumberParameter(params, step, "jump_velocity", p.jumpVelocity);
    return p;
}

void UpdateFpsCrouchHeight(WorkflowContext& context, bool crouching,
                           float crouchHeight, float standHeight, float dt) {
    const float target  = crouching ? crouchHeight : standHeight;
    const float current = context.Get<float>("camera_eye_height", standHeight);
    constexpr float kLerpSpeedPerSecond = 8.0f;
    const float t = std::min(kLerpSpeedPerSecond * dt, 1.0f);
    context.Set<float>("camera_eye_height", current + (target - current) * t);
}

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

void TryFpsStepUp(btRigidBody* body, btDiscreteDynamicsWorld* world,
                  const FpsWishVelocity& wish, float stepHeight, float dt,
                  float& accumulatorSeconds) {
    constexpr float kStepInterval = 1.0f / 60.0f;
    accumulatorSeconds += dt;
    const bool stepReady = accumulatorSeconds >= kStepInterval;

    btVector3 dir(wish.x, 0.0f, wish.z);
    const float dirMag = dir.length();
    if (!stepReady || !world || dirMag <= 0.0001f || stepHeight <= 0.0f) {
        return;
    }
    accumulatorSeconds = 0.0f;
    dir /= dirMag;

    const float capsuleRadius = 0.3f;  // matches q3_game.json player shape
    const float capsuleHalfH  = 0.5f;  // height/2
    btTransform xform;
    body->getMotionState()->getWorldTransform(xform);
    const btVector3 origin = xform.getOrigin();
    const float feetY      = origin.y() - capsuleHalfH - capsuleRadius;

    // Start probes just outside the capsule shell so they don't hit it.
    // probeReach is the lookahead: bigger means the snap-up detects stairs
    // sooner, before the capsule is fully wedged against a riser.
    const float probeStart = capsuleRadius + 0.05f;
    const float probeReach = 0.70f;

    // 1. Low probe: is something blocking us at shin height?
    const btVector3 lowFrom =
        origin + dir * probeStart -
        btVector3(0, capsuleHalfH + capsuleRadius - 0.10f, 0);
    const btVector3 lowTo = lowFrom + dir * probeReach;
    btCollisionWorld::ClosestRayResultCallback lowHit(lowFrom, lowTo);
    world->rayTest(lowFrom, lowTo, lowHit);
    if (!lowHit.hasHit()) {
        return;
    }

    // 2. High probe: is the path clear at step_height?
    const btVector3 highFrom = lowFrom + btVector3(0, stepHeight + 0.05f, 0);
    const btVector3 highTo   = highFrom + dir * probeReach;
    btCollisionWorld::ClosestRayResultCallback highHit(highFrom, highTo);
    world->rayTest(highFrom, highTo, highHit);
    if (highHit.hasHit()) {
        return;
    }

    // 3. Down probe: find the top surface to step onto.
    const btVector3 downFrom = lowTo + btVector3(0, stepHeight, 0);
    const btVector3 downTo =
        btVector3(downFrom.x(), feetY - 0.05f, downFrom.z());
    btCollisionWorld::ClosestRayResultCallback downHit(downFrom, downTo);
    world->rayTest(downFrom, downTo, downHit);
    if (!downHit.hasHit()) {
        return;
    }

    const float newFeetY = downHit.m_hitPointWorld.y();
    const float deltaY   = newFeetY - feetY;
    if (deltaY <= 0.05f || deltaY >= stepHeight) {
        return;
    }

    // Snap up AND forward. The forward nudge places the capsule fully onto
    // the new tread so it does not immediately snag the next riser; without
    // it, each step costs a frame of solver-fighting and feels
    // staircase-shaped instead of ramp-shaped. Horizontal velocity is left
    // alone so movement up the staircase stays smooth.
    const float forwardNudge = capsuleRadius + 0.05f;
    xform.setOrigin(origin + btVector3(0, deltaY + 0.02f, 0) +
                    dir * forwardNudge);
    body->setWorldTransform(xform);
    body->getMotionState()->setWorldTransform(xform);
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
