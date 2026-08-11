#include "services/interfaces/workflow/workflow_generic_steps/workflow_physics_world_create_step.hpp"
#include "services/interfaces/workflow/workflow_step_parameter_resolver.hpp"
#include "services/interfaces/workflow_step_definition.hpp"
#include "services/interfaces/workflow_context.hpp"

#include <btBulletDynamicsCommon.h>
#include <nlohmann/json.hpp>
#include <stdexcept>
#include <utility>

namespace sdl3cpp::services::impl {

WorkflowPhysicsWorldCreateStep::WorkflowPhysicsWorldCreateStep(std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)) {}

std::string WorkflowPhysicsWorldCreateStep::GetPluginId() const {
    return "physics.world.create";
}

void WorkflowPhysicsWorldCreateStep::Execute(
    const WorkflowStepDefinition& step, WorkflowContext& context) {

    WorkflowStepParameterResolver paramResolver;

    float gravity_x = 0.0f, gravity_y = -9.81f, gravity_z = 0.0f;

    auto readParam = [&](const char* name, auto& out) {
        if (const auto* p = paramResolver.FindParameter(step, name)) {
            if (p->type == WorkflowParameterValue::Type::Number) {
                out = static_cast<std::remove_reference_t<decltype(out)>>(p->numberValue);
            }
        }
    };

    readParam("gravity_x", gravity_x);
    readParam("gravity_y", gravity_y);
    readParam("gravity_z", gravity_z);

    // Bullet broadphase, dispatcher, solver, world
    auto* broadphase = new btDbvtBroadphase();
    auto* collisionConfig = new btDefaultCollisionConfiguration();
    auto* dispatcher = new btCollisionDispatcher(collisionConfig);
    auto* solver = new btSequentialImpulseConstraintSolver();

    auto* world = new btDiscreteDynamicsWorld(dispatcher, broadphase, solver, collisionConfig);
    world->setGravity(btVector3(gravity_x, gravity_y, gravity_z));

    // Store in context
    context.Set<btDiscreteDynamicsWorld*>("physics_world", world);
    context.Set<btDbvtBroadphase*>("physics_broadphase", broadphase);
    context.Set<btDefaultCollisionConfiguration*>("physics_collision_config", collisionConfig);
    context.Set<btCollisionDispatcher*>("physics_dispatcher", dispatcher);
    context.Set<btSequentialImpulseConstraintSolver*>("physics_solver", solver);

    // Initialize body registry
    context.Set("physics_bodies", nlohmann::json::array());

    if (logger_) {
        logger_->Info("physics.world.create: World created, gravity=(" +
                      std::to_string(gravity_x) + "," +
                      std::to_string(gravity_y) + "," +
                      std::to_string(gravity_z) + ")");
    }
}

}  // namespace sdl3cpp::services::impl
