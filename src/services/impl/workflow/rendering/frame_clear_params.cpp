#include "services/interfaces/workflow/rendering/frame_clear_params.hpp"
#include "services/interfaces/workflow/workflow_step_parameter_resolver.hpp"

namespace sdl3cpp::services::impl {

ClearColorParams ReadClearColorParams(const WorkflowStepDefinition& step) {
    WorkflowStepParameterResolver paramResolver;
    ClearColorParams out;
    auto readParam = [&](const char* name, float& value) {
        if (const auto* p = paramResolver.FindParameter(step, name)) {
            if (p->type == WorkflowParameterValue::Type::Number) {
                value = static_cast<float>(p->numberValue);
            }
        }
    };
    readParam("clear_r", out.r);
    readParam("clear_g", out.g);
    readParam("clear_b", out.b);
    return out;
}

}  // namespace sdl3cpp::services::impl
