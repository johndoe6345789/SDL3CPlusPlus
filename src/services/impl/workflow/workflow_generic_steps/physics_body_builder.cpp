#include "services/interfaces/workflow/workflow_generic_steps/physics_body_builder.hpp"
#include "services/interfaces/workflow/workflow_step_parameter_resolver.hpp"

#include <type_traits>

namespace sdl3cpp::services::impl {

PhysicsBodyParams ResolvePhysicsBodyParams(
    const WorkflowStepDefinition& step) {
    WorkflowStepParameterResolver resolver;
    PhysicsBodyParams params;

    if (const auto* p = resolver.FindParameter(step, "name")) {
        if (p->type == WorkflowParameterValue::Type::String) {
            params.name = p->stringValue;
        }
    }
    if (const auto* p = resolver.FindParameter(step, "shape")) {
        if (p->type == WorkflowParameterValue::Type::String) {
            params.shape = p->stringValue;
        }
    }

    auto readNum = [&](const char* pname, auto& out) {
        if (const auto* p = resolver.FindParameter(step, pname)) {
            if (p->type == WorkflowParameterValue::Type::Number) {
                out = static_cast<std::remove_reference_t<decltype(out)>>(
                    p->numberValue);
            }
        }
    };
    readNum("mass", params.mass);
    readNum("pos_x", params.pos_x);
    readNum("pos_y", params.pos_y);
    readNum("pos_z", params.pos_z);
    readNum("size_x", params.size_x);
    readNum("size_y", params.size_y);
    readNum("size_z", params.size_z);
    readNum("radius", params.radius);
    readNum("height", params.height);
    readNum("lock_rotation", params.lock_rotation);
    readNum("is_player", params.is_player);
    readNum("spinning", params.spinning);
    readNum("spin_speed_x", params.spin_speed_x);
    readNum("spin_speed_y", params.spin_speed_y);
    readNum("visible", params.visible);
    return params;
}

PhysicsBody BuildPhysicsBody(btDiscreteDynamicsWorld* world,
                             const PhysicsBodyParams& p) {
    PhysicsBody out;

    if (p.shape == "capsule") {
        out.shape = new btCapsuleShape(p.radius, p.height);
        out.visual = {
            {"scale", {p.radius * 2.0f, (p.height + p.radius * 2.0f) / 2.0f,
                      p.radius * 2.0f}},
            {"visible", p.is_player < 0.5f},
            {"spinning", false}};
    } else {
        // Default: box
        btVector3 halfExtents(p.size_x / 2.0f, p.size_y / 2.0f,
                              p.size_z / 2.0f);
        out.shape = new btBoxShape(halfExtents);
        out.visual = {{"scale", {p.size_x / 2.0f, p.size_y / 2.0f,
                                 p.size_z / 2.0f}},
                      {"visible", p.visible > 0.5f},
                      {"spinning", p.spinning > 0.5f},
                      {"spin_speed_x", p.spin_speed_x},
                      {"spin_speed_y", p.spin_speed_y}};
    }

    btTransform startTransform;
    startTransform.setIdentity();
    startTransform.setOrigin(btVector3(p.pos_x, p.pos_y, p.pos_z));

    btVector3 localInertia(0, 0, 0);
    if (p.mass > 0.0f) {
        out.shape->calculateLocalInertia(p.mass, localInertia);
    }

    auto* motionState = new btDefaultMotionState(startTransform);
    btRigidBody::btRigidBodyConstructionInfo rbInfo(p.mass, motionState,
                                                    out.shape, localInertia);
    out.body = new btRigidBody(rbInfo);

    // Lock rotation for player/character bodies
    if (p.lock_rotation > 0.5f) {
        out.body->setAngularFactor(btVector3(0, 0, 0));
        out.body->setFriction(0.5f);
        out.body->setActivationState(DISABLE_DEACTIVATION);
    }

    // Static bodies don't need deactivation management
    if (p.mass == 0.0f) {
        out.body->setFriction(1.0f);
    }

    world->addRigidBody(out.body);
    return out;
}

}  // namespace sdl3cpp::services::impl
