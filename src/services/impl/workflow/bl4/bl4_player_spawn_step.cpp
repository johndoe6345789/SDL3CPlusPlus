#include "services/interfaces/workflow/bl4/bl4_player_steps.hpp"

#include "services/interfaces/workflow/bl4/bl4_step_params.hpp"
#include "services/interfaces/workflow/quake3/pmove/q3_pm_types.hpp"

#include <utility>

namespace sdl3cpp::services::impl {
namespace {

// Forward (-sin yaw, -cos yaw) against compass (sin h, -cos h); the same
// convention fs2024's Fs2024YawForHeading uses.
float Bl4YawForHeading(float headingDegrees) {
    return -headingDegrees * 3.14159265f / 180.f;
}

void Bl4MoveBody(btRigidBody* body, const glm::vec3& origin) {
    if (!body) return;
    btTransform transform = body->getWorldTransform();
    transform.setOrigin(btVector3(origin.x, origin.y, origin.z));
    body->setWorldTransform(transform);
    if (auto* motion = body->getMotionState()) motion->setWorldTransform(transform);
    body->setLinearVelocity(btVector3(0, 0, 0));
    body->setAngularVelocity(btVector3(0, 0, 0));
}

}  // namespace

WorkflowBl4PlayerSpawnStep::WorkflowBl4PlayerSpawnStep(std::shared_ptr<ILogger> logger,
                                                      std::shared_ptr<Bl4TileStreamState> state)
    : logger_(std::move(logger)), state_(std::move(state)) {}

std::string WorkflowBl4PlayerSpawnStep::GetPluginId() const { return "bl4.player.spawn"; }

void WorkflowBl4PlayerSpawnStep::Execute(const WorkflowStepDefinition& step,
                                        WorkflowContext& context) {
    const glm::vec3 origin{Bl4NumberOrEnv(step, "x", 0.f), Bl4NumberOrEnv(step, "y", 0.f),
                           Bl4NumberOrEnv(step, "z", 0.f)};
    const float heading = Bl4NumberOrEnv(step, "heading", 0.f);

    const auto name = context.GetString("physics_player_body", "");
    Bl4MoveBody(name.empty() ? nullptr : context.Get<btRigidBody*>("physics_body_" + name, nullptr),
               origin);
    if (auto ps = context.TryGet<Q3PlayerState>("q3.ps")) {
        Q3PlayerState moved = *ps;
        moved.origin = origin;
        moved.velocity = glm::vec3(0.f);
        context.Set("q3.ps", moved);
    }
    context.Set<glm::vec3>("bl4.spawn_origin", origin);  // bl4.player.respawn's fallback
    context.Set<float>("camera_yaw", Bl4YawForHeading(heading));
    context.Set<float>("camera_pitch", Bl4NumberOrEnv(step, "pitch", 0.f) * 3.14159265f / 180.f);

    if (logger_) {
        logger_->Info("bl4.player.spawn: (" + std::to_string(origin.x) + ", " +
                      std::to_string(origin.y) + ", " + std::to_string(origin.z) + ") heading " +
                      std::to_string(heading));
    }
}

}  // namespace sdl3cpp::services::impl
