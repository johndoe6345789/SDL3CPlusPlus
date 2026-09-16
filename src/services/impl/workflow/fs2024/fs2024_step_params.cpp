#include "services/interfaces/workflow/fs2024/fs2024_step_params.hpp"

namespace sdl3cpp::services::impl {

std::string Fs2024StringOr(const WorkflowStepDefinition& step,
                           const std::string& name,
                           const std::string& fallback) {
    const auto it = step.parameters.find(name);
    if (it == step.parameters.end() ||
        it->second.type != WorkflowParameterValue::Type::String) {
        return fallback;
    }
    return it->second.stringValue;
}

float Fs2024NumberOr(const WorkflowStepDefinition& step,
                     const std::string& name, float fallback) {
    const auto it = step.parameters.find(name);
    if (it == step.parameters.end() ||
        it->second.type != WorkflowParameterValue::Type::Number) {
        return fallback;
    }
    return static_cast<float>(it->second.numberValue);
}

}  // namespace sdl3cpp::services::impl
