#include "services/interfaces/workflow/workflow_generic_steps/physics_body_params.hpp"
#include "services/interfaces/workflow/workflow_step_parameter_resolver.hpp"

#include <type_traits>

namespace sdl3cpp::services::impl {

PhysicsBodyParams ResolvePhysicsBodyParams(const WorkflowStepDefinition& step) {
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

}  // namespace sdl3cpp::services::impl
