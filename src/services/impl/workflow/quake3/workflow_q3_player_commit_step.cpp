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

    btTransform transform = body->getWorldTransform();
    transform.setOrigin(btVector3(ps->origin.x, ps->origin.y, ps->origin.z));
    body->setWorldTransform(transform);
    if (auto* motion = body->getMotionState()) {
        motion->setWorldTransform(transform);
    }

    // The pmove model integrates velocity itself; leaving Bullet's copy
    // set would double-apply it and accumulate gravity the traces have
    // already resolved.
    body->setLinearVelocity(btVector3(0, 0, 0));
    body->setAngularVelocity(btVector3(0, 0, 0));
}

}  // namespace sdl3cpp::services::impl
