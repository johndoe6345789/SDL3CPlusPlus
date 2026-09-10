#include "services/interfaces/workflow/gta5/gta5_step_params.hpp"

namespace sdl3cpp::services::impl {

std::string Gta5ParameterOr(const WorkflowStepDefinition& step,
                            const std::string& name,
                            const std::string& fallback) {
    const auto it = step.parameters.find(name);
    if (it == step.parameters.end() ||
        it->second.type != WorkflowParameterValue::Type::String) {
        return fallback;
    }
    return it->second.stringValue;
}

int Gta5ParameterOrInt(const WorkflowStepDefinition& step,
                       const std::string& name, int fallback) {
    const auto it = step.parameters.find(name);
    if (it == step.parameters.end() ||
        it->second.type != WorkflowParameterValue::Type::Number) {
        return fallback;
    }
    return static_cast<int>(it->second.numberValue);
}

}  // namespace sdl3cpp::services::impl
