#include "services/interfaces/workflow/workflow_generic_steps/workflow_physics_body_add_step.hpp"
#include "services/interfaces/workflow/workflow_step_parameter_resolver.hpp"
#include "services/interfaces/workflow_step_definition.hpp"
#include "services/interfaces/workflow_context.hpp"

#include <btBulletDynamicsCommon.h>
#include <nlohmann/json.hpp>
#include <stdexcept>
#include <utility>
#include <string>

namespace sdl3cpp::services::impl {

WorkflowPhysicsBodyAddStep::WorkflowPhysicsBodyAddStep(std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)) {}

std::string WorkflowPhysicsBodyAddStep::GetPluginId() const {
    return "physics.body.add";
}

void WorkflowPhysicsBodyAddStep::Execute(
    const WorkflowStepDefinition& step, WorkflowContext& context) {

    auto* world = context.Get<btDiscreteDynamicsWorld*>("physics_world", nullptr);
    if (!world) {
        throw std::runtime_error("physics.body.add: No physics world (run physics.world.create first)");
    }

    WorkflowStepParameterResolver paramResolver;

    // Body parameters
    std::string name = "body";
    std::string shape = "box";
    float mass = 0.0f;
    float pos_x = 0.0f, pos_y = 0.0f, pos_z = 0.0f;
    // Box dimensions
    float size_x = 1.0f, size_y = 1.0f, size_z = 1.0f;
    // Capsule dimensions
    float radius = 0.4f, height = 1.2f;
    // Flags
    float lock_rotation = 0.0f;
    float is_player = 0.0f;
    float spinning = 0.0f;
    float spin_speed_x = 1.0f, spin_speed_y = 0.7f;
    float visible = 1.0f;

    // Read string params
    if (const auto* p = paramResolver.FindParameter(step, "name")) {
        if (p->type == WorkflowParameterValue::Type::String) name = p->stringValue;
    }
    if (const auto* p = paramResolver.FindParameter(step, "shape")) {
        if (p->type == WorkflowParameterValue::Type::String) shape = p->stringValue;
    }

    auto readNum = [&](const char* pname, auto& out) {
        if (const auto* p = paramResolver.FindParameter(step, pname)) {
            if (p->type == WorkflowParameterValue::Type::Number) {
                out = static_cast<std::remove_reference_t<decltype(out)>>(p->numberValue);
            }
        }
    };

    readNum("mass", mass);
    readNum("pos_x", pos_x);
    readNum("pos_y", pos_y);
    readNum("pos_z", pos_z);
    readNum("size_x", size_x);
    readNum("size_y", size_y);
    readNum("size_z", size_z);
    readNum("radius", radius);
    readNum("height", height);
    readNum("lock_rotation", lock_rotation);
    readNum("is_player", is_player);
    readNum("spinning", spinning);
    readNum("spin_speed_x", spin_speed_x);
    readNum("spin_speed_y", spin_speed_y);
    readNum("visible", visible);

    // Create collision shape
    btCollisionShape* collisionShape = nullptr;
    nlohmann::json visual;

    if (shape == "capsule") {
        collisionShape = new btCapsuleShape(radius, height);
        visual = {
            {"scale", {radius * 2.0f, (height + radius * 2.0f) / 2.0f, radius * 2.0f}},
            {"visible", is_player < 0.5f},
            {"spinning", false}
        };
    } else {
        // Default: box
        btVector3 halfExtents(size_x / 2.0f, size_y / 2.0f, size_z / 2.0f);
        collisionShape = new btBoxShape(halfExtents);
        visual = {
            {"scale", {size_x / 2.0f, size_y / 2.0f, size_z / 2.0f}},
            {"visible", visible > 0.5f},
            {"spinning", spinning > 0.5f},
            {"spin_speed_x", spin_speed_x},
            {"spin_speed_y", spin_speed_y}
        };
    }

    // Create rigid body
    btTransform startTransform;
    startTransform.setIdentity();
    startTransform.setOrigin(btVector3(pos_x, pos_y, pos_z));

    btVector3 localInertia(0, 0, 0);
    if (mass > 0.0f) {
        collisionShape->calculateLocalInertia(mass, localInertia);
    }

    auto* motionState = new btDefaultMotionState(startTransform);
    btRigidBody::btRigidBodyConstructionInfo rbInfo(mass, motionState, collisionShape, localInertia);
    auto* body = new btRigidBody(rbInfo);

    // Lock rotation for player/character bodies
    if (lock_rotation > 0.5f) {
        body->setAngularFactor(btVector3(0, 0, 0));
        body->setFriction(0.5f);
        body->setActivationState(DISABLE_DEACTIVATION);
    }

    // Static bodies don't need deactivation management
    if (mass == 0.0f) {
        body->setFriction(1.0f);
    }

    world->addRigidBody(body);

    // Store in context
    context.Set<btRigidBody*>("physics_body_" + name, body);
    context.Set<btCollisionShape*>("physics_shape_" + name, collisionShape);
    context.Set("physics_visual_" + name, visual);

    // Add to body registry
    auto bodies = context.Get<nlohmann::json>("physics_bodies", nlohmann::json::array());
    bodies.push_back(name);
    context.Set("physics_bodies", bodies);

    if (is_player > 0.5f) {
        context.Set<std::string>("physics_player_body", name);
    }

    if (logger_) {
        logger_->Info("physics.body.add: '" + name + "' shape=" + shape +
                      " mass=" + std::to_string(mass) +
                      " pos=(" + std::to_string(pos_x) + "," +
                      std::to_string(pos_y) + "," + std::to_string(pos_z) + ")");
    }
}

}  // namespace sdl3cpp::services::impl
