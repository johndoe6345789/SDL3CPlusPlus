#include "services/interfaces/workflow/bl4/bl4_step_params.hpp"

namespace sdl3cpp::services::impl {

std::string Bl4StringOr(const WorkflowStepDefinition& step,
                        const std::string& name, const std::string& fallback) {
    const auto it = step.parameters.find(name);
    if (it == step.parameters.end() ||
        it->second.type != WorkflowParameterValue::Type::String) {
        return fallback;
    }
    return it->second.stringValue;
}

float Bl4NumberOr(const WorkflowStepDefinition& step, const std::string& name,
                  float fallback) {
    const auto it = step.parameters.find(name);
    if (it == step.parameters.end() ||
        it->second.type != WorkflowParameterValue::Type::Number) {
        return fallback;
    }
    return static_cast<float>(it->second.numberValue);
}

}  // namespace sdl3cpp::services::impl
