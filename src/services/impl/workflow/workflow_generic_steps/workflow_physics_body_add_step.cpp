#include "services/interfaces/workflow/workflow_generic_steps/workflow_physics_body_add_step.hpp"
#include "services/interfaces/workflow/workflow_generic_steps/physics_body_builder.hpp"
#include "services/interfaces/workflow_context.hpp"
#include "services/interfaces/workflow_step_definition.hpp"

#include <btBulletDynamicsCommon.h>
#include <nlohmann/json.hpp>

#include <stdexcept>
#include <string>
#include <utility>

namespace sdl3cpp::services::impl {

WorkflowPhysicsBodyAddStep::WorkflowPhysicsBodyAddStep(
    std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)) {}

std::string WorkflowPhysicsBodyAddStep::GetPluginId() const {
    return "physics.body.add";
}

void WorkflowPhysicsBodyAddStep::Execute(const WorkflowStepDefinition& step,
                                         WorkflowContext& context) {
    auto* world =
        context.Get<btDiscreteDynamicsWorld*>("physics_world", nullptr);
    if (!world) {
        throw std::runtime_error(
            "physics.body.add: No physics world (run physics.world.create "
            "first)");
    }

    const PhysicsBodyParams params = ResolvePhysicsBodyParams(step);
    const PhysicsBody built = BuildPhysicsBody(world, params);
    const std::string& name = params.name;

    context.Set<btRigidBody*>("physics_body_" + name, built.body);
    context.Set<btCollisionShape*>("physics_shape_" + name, built.shape);
    context.Set("physics_visual_" + name, built.visual);

    auto bodies =
        context.Get<nlohmann::json>("physics_bodies", nlohmann::json::array());
    bodies.push_back(name);
    context.Set("physics_bodies", bodies);

    if (params.is_player > 0.5f) {
        context.Set<std::string>("physics_player_body", name);
    }

    if (logger_) {
        logger_->Info(
            "physics.body.add: '" + name + "' shape=" + params.shape +
            " mass=" + std::to_string(params.mass) + " pos=(" +
            std::to_string(params.pos_x) + "," +
            std::to_string(params.pos_y) + "," +
            std::to_string(params.pos_z) + ")");
    }
}

}  // namespace sdl3cpp::services::impl
