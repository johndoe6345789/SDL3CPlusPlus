#include "services/interfaces/workflow/rendering/workflow_spawn_apply_step.hpp"

#include <btBulletDynamicsCommon.h>
#include <nlohmann/json.hpp>

namespace sdl3cpp::services::impl {

WorkflowSpawnApplyStep::WorkflowSpawnApplyStep(std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)) {}

std::string WorkflowSpawnApplyStep::GetPluginId() const {
    return "spawn.apply";
}

void WorkflowSpawnApplyStep::Execute(const WorkflowStepDefinition& step, WorkflowContext& context) {
    const auto* spawn = context.TryGet<nlohmann::json>("bsp.spawn");
    if (!spawn) return;

    auto playerName = context.GetString("physics_player_body", "");
    if (playerName.empty()) return;

    auto* body = context.Get<btRigidBody*>("physics_body_" + playerName, nullptr);
    if (!body) return;

    float x = spawn->value("x", 0.0f);
    float y = spawn->value("y", 5.0f);
    float z = spawn->value("z", 0.0f);
    float angle = spawn->value("angle", 0.0f);

    btTransform transform;
    transform.setIdentity();
    transform.setOrigin(btVector3(x, y, z));
    body->setWorldTransform(transform);
    body->getMotionState()->setWorldTransform(transform);
    body->setLinearVelocity(btVector3(0, 0, 0));
    body->setAngularVelocity(btVector3(0, 0, 0));

    // Convert Q3 angle (degrees, 0=east) to engine yaw (radians)
    float yaw = angle * 3.14159265f / 180.0f;
    context.Set<float>("camera_yaw", yaw);

    if (logger_) {
        logger_->Info("spawn.apply: Player at (" + std::to_string(x) + ", " +
                     std::to_string(y) + ", " + std::to_string(z) +
                     ") yaw=" + std::to_string(yaw));
    }
}

}
