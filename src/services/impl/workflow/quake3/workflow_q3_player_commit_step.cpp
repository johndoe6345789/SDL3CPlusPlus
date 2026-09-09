#include "services/interfaces/workflow/quake3/workflow_q3_player_commit_step.hpp"
#include "services/interfaces/workflow/quake3/q3_pm_types.hpp"

#include <btBulletDynamicsCommon.h>

namespace sdl3cpp::services::impl {

WorkflowQ3PlayerCommitStep::WorkflowQ3PlayerCommitStep(std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)) {}

std::string WorkflowQ3PlayerCommitStep::GetPluginId() const {
    return "q3.player.commit";
}

void WorkflowQ3PlayerCommitStep::Execute(const WorkflowStepDefinition&,
                                         WorkflowContext& context) {
    const auto* ps = context.TryGet<Q3PlayerState>("q3.ps");
    if (!ps) {
        return;
    }

    const auto playerName = context.GetString("physics_player_body", "");
    if (playerName.empty()) {
        return;
    }
    auto* body = context.Get<btRigidBody*>("physics_body_" + playerName,
                                           nullptr);
    if (!body) {
        return;
    }

    // The pmove model traces and integrates the player itself, so Bullet
    // must not also simulate this body: left dynamic it accumulates
    // gravity every step and the body sinks away from the traced
    // position, which reads as juddering and eventually falling out of
    // the map. Clearing gravity and velocity leaves it a collision proxy
    // that other bodies can still hit.
    body->setGravity(btVector3(0, 0, 0));
    body->setLinearVelocity(btVector3(0, 0, 0));
    body->setAngularVelocity(btVector3(0, 0, 0));
    body->setActivationState(DISABLE_DEACTIVATION);

    btTransform transform = body->getWorldTransform();
    transform.setOrigin(btVector3(ps->origin.x, ps->origin.y, ps->origin.z));
    body->setWorldTransform(transform);
    if (auto* motion = body->getMotionState()) {
        motion->setWorldTransform(transform);
    }
}

}  // namespace sdl3cpp::services::impl
