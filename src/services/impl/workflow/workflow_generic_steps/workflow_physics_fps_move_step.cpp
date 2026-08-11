#include "services/interfaces/workflow/workflow_generic_steps/workflow_physics_fps_move_step.hpp"
#include "services/interfaces/workflow/workflow_step_parameter_resolver.hpp"
#include "services/interfaces/workflow_step_definition.hpp"
#include "services/interfaces/workflow_context.hpp"

#include <btBulletDynamicsCommon.h>
#include <cmath>
#include <stdexcept>
#include <utility>

namespace sdl3cpp::services::impl {

WorkflowPhysicsFpsMoveStep::WorkflowPhysicsFpsMoveStep(std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)) {}

std::string WorkflowPhysicsFpsMoveStep::GetPluginId() const {
    return "physics.fps.move";
}

void WorkflowPhysicsFpsMoveStep::Execute(
    const WorkflowStepDefinition& step, WorkflowContext& context) {

    // Get player body
    auto playerName = context.GetString("physics_player_body", "");
    if (playerName.empty()) return;

    auto* body = context.Get<btRigidBody*>("physics_body_" + playerName, nullptr);
    if (!body) return;
    if (!context.GetBool("movement_active", true)) {
        btVector3 vel = body->getLinearVelocity();
        body->setLinearVelocity(btVector3(0, vel.y(), 0));
        return;
    }

    // Read parameters from workflow JSON
    WorkflowStepParameterResolver paramResolver;
    auto getNum = [&](const char* name, float def) -> float {
        const auto* p = paramResolver.FindParameter(step, name);
        return (p && p->type == WorkflowParameterValue::Type::Number) ? static_cast<float>(p->numberValue) : def;
    };

    const float moveSpeed = getNum("move_speed", 6.0f);
    const float sprintMultiplier = getNum("sprint_multiplier", 1.8f);
    const float crouchMultiplier = getNum("crouch_multiplier", 0.4f);
    const float crouchHeight = getNum("crouch_height", 0.8f);
    const float standHeight = getNum("stand_height", 1.6f);
    const float airControl = getNum("air_control", 0.3f);
    const float gravityScale = getNum("gravity_scale", 1.0f);
    // Acceleration model (CalcFriction-style). Velocity accelerates toward the
    // target each frame instead of snapping. Equal accel/friction = modern feel.
    const float groundAccel = getNum("ground_accel", 35.0f);
    const float groundFriction = getNum("ground_friction", 30.0f);
    const float dtMove = context.Get<float>("physics_dt", 1.0f / 60.0f);

    // Read input state from context (set by input.poll)
    bool keyW = context.GetBool("input_key_w", false);
    bool keyA = context.GetBool("input_key_a", false);
    bool keyS = context.GetBool("input_key_s", false);
    bool keyD = context.GetBool("input_key_d", false);
    bool keySpace = context.GetBool("input_key_space", false);
    bool keyShift = context.GetBool("input_key_shift", false);
    bool keyCtrl = context.GetBool("input_key_ctrl", false);

    // Read camera yaw (set by camera.fps.update from previous frame)
    float yaw = context.Get<float>("camera_yaw", 0.0f);

    // Calculate forward/right vectors from yaw
    float sinY = std::sin(yaw);
    float cosY = std::cos(yaw);
    float forwardX = -sinY;
    float forwardZ = -cosY;
    float rightX = cosY;
    float rightZ = -sinY;

    // Build movement direction
    float moveX = 0.0f, moveZ = 0.0f;
    if (keyW) { moveX += forwardX; moveZ += forwardZ; }
    if (keyS) { moveX -= forwardX; moveZ -= forwardZ; }
    if (keyA) { moveX -= rightX;   moveZ -= rightZ; }
    if (keyD) { moveX += rightX;   moveZ += rightZ; }

    // Apply sprint/crouch speed modifiers
    float speed = moveSpeed;
    if (keyCtrl) {
        speed *= crouchMultiplier;
    } else if (keyShift) {
        speed *= sprintMultiplier;
    }

    // Normalize horizontal movement
    float len = std::sqrt(moveX * moveX + moveZ * moveZ);
    if (len > 0.001f) {
        moveX = (moveX / len) * speed;
        moveZ = (moveZ / len) * speed;
    }

    // Check grounded state via raycast
    btVector3 currentVel = body->getLinearVelocity();
    bool grounded = false;
    auto* world = context.Get<btDiscreteDynamicsWorld*>("physics_world", nullptr);
    if (world) {
        btTransform bodyTransform;
        body->getMotionState()->getWorldTransform(bodyTransform);
        btVector3 from = bodyTransform.getOrigin();
        btVector3 to = from + btVector3(0, -1.2f, 0);
        btCollisionWorld::ClosestRayResultCallback rayResult(from, to);
        world->rayTest(from, to, rayResult);
        grounded = rayResult.hasHit();
    }

    if (grounded) {
        // Inertia model: accelerate horizontal velocity toward target instead of
        // snap-setting it. When input released, friction decelerates to zero.
        float horizX = currentVel.x();
        float horizZ = currentVel.z();
        if (len > 0.001f) {
            float diffX = moveX - horizX;
            float diffZ = moveZ - horizZ;
            float diffLen = std::sqrt(diffX * diffX + diffZ * diffZ);
            float maxStep = groundAccel * dtMove;
            if (diffLen > maxStep && diffLen > 0.0f) {
                float k = maxStep / diffLen;
                diffX *= k;
                diffZ *= k;
            }
            horizX += diffX;
            horizZ += diffZ;
        } else {
            float curSpeed = std::sqrt(horizX * horizX + horizZ * horizZ);
            if (curSpeed > 0.001f) {
                float drop = std::min(curSpeed, groundFriction * dtMove);
                float k = (curSpeed - drop) / curSpeed;
                horizX *= k;
                horizZ *= k;
            }
        }
        body->setLinearVelocity(btVector3(horizX, currentVel.y(), horizZ));
    } else {
        // Air control: blend input with current horizontal velocity (Quake-style)
        float curX = currentVel.x();
        float curZ = currentVel.z();
        float newX = curX + (moveX - curX) * airControl;
        float newZ = curZ + (moveZ - curZ) * airControl;
        body->setLinearVelocity(btVector3(newX, currentVel.y(), newZ));
    }

    // Per-player gravity scaling (applies symmetrically in air, both rising
    // and falling). gravityScale > 1.0 = heavier; < 1.0 = floatier; 1.0 =
    // Earth. Implemented as an impulse (force * dt) rather than applyCentralForce
    // so the integrated effect is identical regardless of framerate. Bullet
    // accumulates forces across frames between substeps, so a per-frame force
    // call at 240Hz would otherwise multiply 4×.
    if (!grounded && gravityScale != 1.0f) {
        const float gravImpulse = -9.81f * body->getMass() * (gravityScale - 1.0f) * dtMove;
        body->applyCentralImpulse(btVector3(0, gravImpulse, 0));
    }

    // Step-up: capsule rigid bodies can't roll over Q3 stairs (~0.5m tall vs
    // 0.3m capsule radius). Each frame, while grounded and trying to move
    // forward, do three raycasts to detect a low obstacle with a walkable
    // surface on top, then teleport the body up. This is the "step trace"
    // pattern Q3's PM_StepSlideMove uses, simplified to rays.
    const float stepHeight = getNum("step_height", 0.55f);
    // Rate-limit step-up to ~60Hz. At 240Hz, calling step-up every frame would
    // multi-teleport (forward nudge stacks, ditto vertical snap), making the
    // player either rocket past stairs or stutter-stick on the last few steps.
    // Accumulate dt and only attempt step-up once we've banked a 60Hz tick.
    constexpr float kStepInterval = 1.0f / 60.0f;
    step_accumulator_s_ += dtMove;
    const bool stepReady = step_accumulator_s_ >= kStepInterval;

    btVector3 dir(moveX, 0.0f, moveZ);
    const float dirMag = dir.length();
    if (stepReady && grounded && dirMag > 0.0001f && world && stepHeight > 0.0f) {
        step_accumulator_s_ = 0.0f;
        dir /= dirMag;
        const float capsuleRadius = 0.3f;       // matches q3_game.json player shape
        const float capsuleHalfH  = 0.5f;       // height/2
        btTransform xform;
        body->getMotionState()->getWorldTransform(xform);
        const btVector3 origin = xform.getOrigin();
        const float feetY = origin.y() - capsuleHalfH - capsuleRadius;

        // Start probes just outside the capsule shell so they don't hit it.
        // probeReach is the lookahead — bigger = detect stairs sooner so the
        // snap-up happens before the capsule is fully wedged against a riser.
        const float probeStart = capsuleRadius + 0.05f;
        const float probeReach = 0.70f;

        // 1. Low probe — is something blocking us at shin height?
        const btVector3 lowFrom = origin + dir * probeStart - btVector3(0, capsuleHalfH + capsuleRadius - 0.10f, 0);
        const btVector3 lowTo   = lowFrom + dir * probeReach;
        btCollisionWorld::ClosestRayResultCallback lowHit(lowFrom, lowTo);
        world->rayTest(lowFrom, lowTo, lowHit);

        if (lowHit.hasHit()) {
            // 2. High probe — is the path clear at step_height?
            const btVector3 highFrom = lowFrom + btVector3(0, stepHeight + 0.05f, 0);
            const btVector3 highTo   = highFrom + dir * probeReach;
            btCollisionWorld::ClosestRayResultCallback highHit(highFrom, highTo);
            world->rayTest(highFrom, highTo, highHit);

            if (!highHit.hasHit()) {
                // 3. Down probe — find the top surface to step onto.
                const btVector3 downFrom = lowTo + btVector3(0, stepHeight, 0);
                const btVector3 downTo   = btVector3(downFrom.x(), feetY - 0.05f, downFrom.z());
                btCollisionWorld::ClosestRayResultCallback downHit(downFrom, downTo);
                world->rayTest(downFrom, downTo, downHit);

                if (downHit.hasHit()) {
                    const float newFeetY = downHit.m_hitPointWorld.y();
                    const float deltaY   = newFeetY - feetY;
                    if (deltaY > 0.05f && deltaY < stepHeight) {
                        // Snap up AND forward. The forward nudge places the
                        // capsule fully onto the new tread so it doesn't
                        // immediately snag the next riser. Without it, each
                        // step costs a frame of solver-fighting and feels
                        // staircase-shaped instead of ramp-shaped.
                        const float forwardNudge = capsuleRadius + 0.05f;
                        xform.setOrigin(origin
                            + btVector3(0, deltaY + 0.02f, 0)
                            + dir * forwardNudge);
                        body->setWorldTransform(xform);
                        body->getMotionState()->setWorldTransform(xform);
                        // Preserve horizontal velocity so we keep moving up
                        // the staircase smoothly instead of stalling each step.
                    }
                }
            }
        }
    }

    // Jam detection: if the player is pressing movement but the Bullet solver
    // didn't actually move them (hVel << target), they're wedged against a
    // stair riser that the step-up probe missed. After 100ms of being stuck,
    // apply a small upward nudge to lift the capsule clear of the riser.
    // currentVel reflects last frame's realized velocity — the right signal.
    {
        const float hVel = btVector3(currentVel.x(), 0.0f, currentVel.z()).length();
        const bool wantsMove = len > 0.001f;
        if (wantsMove && grounded && hVel < moveSpeed * 0.2f) {
            jam_time_s_ += dtMove;
            if (jam_time_s_ > 0.10f) {
                btTransform xform;
                body->getMotionState()->getWorldTransform(xform);
                xform.setOrigin(xform.getOrigin() + btVector3(0, 0.10f, 0));
                body->setWorldTransform(xform);
                body->getMotionState()->setWorldTransform(xform);
                jam_time_s_ = 0.0f;
            }
        } else {
            jam_time_s_ = 0.0f;
        }
    }

    // Quake-style impulse jump: set upward velocity once on press while
    // grounded; height is determined by gravity from there. No hold-to-extend.
    // The latching `player_jumping` flag prevents auto-bunnyhop from a held key.
    const float jumpVelocity = getNum("jump_velocity", 6.5f);
    bool wasJumping = context.GetBool("player_jumping", false);

    if (keySpace && !keyCtrl && grounded && !wasJumping) {
        btVector3 vel = body->getLinearVelocity();
        body->setLinearVelocity(btVector3(vel.x(), jumpVelocity, vel.z()));
        context.Set<bool>("player_jumping", true);
    }
    if (!keySpace && grounded) {
        context.Set<bool>("player_jumping", false);
    }

    // Crouch: smoothly lerp eye height for natural feel
    float targetHeight = keyCtrl ? crouchHeight : standHeight;
    float currentHeight = context.Get<float>("camera_eye_height", standHeight);
    float lerpSpeed = 8.0f;  // units per second (fast but smooth)
    float dt = context.Get<float>("physics_dt", 1.0f / 60.0f);
    float newHeight = currentHeight + (targetHeight - currentHeight) * std::min(lerpSpeed * dt, 1.0f);
    context.Set<float>("camera_eye_height", newHeight);
    context.Set<bool>("player_crouching", keyCtrl);
    context.Set<bool>("player_sprinting", keyShift && !keyCtrl);

    body->activate(true);
}

}  // namespace sdl3cpp::services::impl
