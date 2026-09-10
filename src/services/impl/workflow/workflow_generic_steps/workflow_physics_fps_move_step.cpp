#include "services/interfaces/workflow/workflow_generic_steps/workflow_physics_fps_move_step.hpp"
#include "services/interfaces/workflow/workflow_generic_steps/physics_fps_move_kinematics.hpp"
#include "services/interfaces/workflow_step_definition.hpp"
#include "services/interfaces/workflow_context.hpp"

#include <btBulletDynamicsCommon.h>
#include <utility>

namespace sdl3cpp::services::impl {

WorkflowPhysicsFpsMoveStep::WorkflowPhysicsFpsMoveStep(
    std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)) {}

std::string WorkflowPhysicsFpsMoveStep::GetPluginId() const {
    return "physics.fps.move";
}

void WorkflowPhysicsFpsMoveStep::Execute(const WorkflowStepDefinition& step,
                                         WorkflowContext& context) {
    const auto playerName = context.GetString("physics_player_body", "");
    if (playerName.empty()) {
        return;
    }
    auto* body =
        context.Get<btRigidBody*>("physics_body_" + playerName, nullptr);
    if (!body) {
        return;
    }
    if (!context.GetBool("movement_active", true)) {
        const btVector3 vel = body->getLinearVelocity();
        body->setLinearVelocity(btVector3(0, vel.y(), 0));
        return;
    }

    const FpsMoveParams params = ReadFpsMoveParams(step);
    const FpsMoveKeys keys     = ReadFpsMoveKeys(context);
    const float dt             = context.Get<float>("physics_dt", 1.0f / 60.0f);
    const float yaw            = context.Get<float>("camera_yaw", 0.0f);

    const FpsWishVelocity wish = ComputeFpsWishVelocity(
        keys, yaw, params.moveSpeed, params.sprintMultiplier,
        params.crouchMultiplier);

    auto* world =
        context.Get<btDiscreteDynamicsWorld*>("physics_world", nullptr);
    const bool grounded = IsFpsBodyGrounded(world, body);

    ApplyFpsMovementVelocity(body, wish, grounded, params.groundAccel,
                             params.groundFriction, params.airControl, dt);
    ApplyFpsGravityScale(body, grounded, params.gravityScale, dt);
    TryFpsStepUp(body, world, wish, params.stepHeight, dt, step_accumulator_s_);
    ApplyFpsJamRecovery(body, body->getLinearVelocity(),
                        wish.inputMagnitude > 0.001f, grounded,
                        params.moveSpeed, dt, jam_time_s_);

    const bool wasJumping = context.GetBool("player_jumping", false);
    context.Set<bool>("player_jumping",
                      ApplyFpsJump(body, keys.jump, keys.crouch, grounded,
                                   wasJumping, params.jumpVelocity));

    UpdateFpsCrouchHeight(context, keys.crouch, params.crouchHeight,
                          params.standHeight, dt);
    context.Set<bool>("player_crouching", keys.crouch);
    context.Set<bool>("player_sprinting", keys.sprint && !keys.crouch);

    body->activate(true);
}

}  // namespace sdl3cpp::services::impl
