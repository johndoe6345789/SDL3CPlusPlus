#include "services/interfaces/workflow/quake3/workflow_q3_player_sync_step.hpp"
#include "services/interfaces/workflow/quake3/q3_pm_types.hpp"

#include <btBulletDynamicsCommon.h>

namespace sdl3cpp::services::impl {

WorkflowQ3PlayerSyncStep::WorkflowQ3PlayerSyncStep(std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)) {}

std::string WorkflowQ3PlayerSyncStep::GetPluginId() const {
    return "q3.player.sync";
}

void WorkflowQ3PlayerSyncStep::Execute(const WorkflowStepDefinition&,
                                       WorkflowContext& context) {
    // Steering: the camera owns yaw and pitch, the pmove chain consumes
    // them. Without this the wish direction never rotates with the view.
    context.Set<float>("q3.player_yaw", context.Get<float>("camera_yaw", 0.f));
    context.Set<float>("q3.player_pitch",
                       context.Get<float>("camera_pitch", 0.f));

    if (context.Contains("q3.ps")) {
        return;
    }

    // First frame after a map loads: seed the state from wherever
    // spawn.apply put the physics body, so both agree on the start.
    Q3PlayerState ps;
    const auto playerName = context.GetString("physics_player_body", "");
    auto* body = playerName.empty()
                     ? nullptr
                     : context.Get<btRigidBody*>("physics_body_" + playerName,
                                                 nullptr);
    if (body) {
        const btVector3& origin = body->getWorldTransform().getOrigin();
        ps.origin = glm::vec3(origin.x(), origin.y(), origin.z());
    }

    context.Set("q3.ps", ps);
    context.Set("q3.player_pos", ps.origin);

    if (logger_) {
        logger_->Info("q3.player.sync: player state created at (" +
                      std::to_string(ps.origin.x) + ", " +
                      std::to_string(ps.origin.y) + ", " +
                      std::to_string(ps.origin.z) + ")");
    }
}

}  // namespace sdl3cpp::services::impl
