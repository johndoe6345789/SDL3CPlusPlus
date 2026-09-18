#include "services/interfaces/workflow/bl4/bl4_step_params.hpp"

#include <cstddef>
#include <exception>

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

float Bl4NumberOrEnv(const WorkflowStepDefinition& step, const std::string& name,
                     float fallback) {
    const std::string text = Bl4StringOr(step, name + "_env", "");
    try {
        std::size_t used = 0;
        const float value = std::stof(text, &used);
        if (used == text.size()) return value;
    } catch (const std::exception&) {
        // Unset (""), or not a number: the workflow's own value stands.
    }
    return Bl4NumberOr(step, name, fallback);
}

}  // namespace sdl3cpp::services::impl
