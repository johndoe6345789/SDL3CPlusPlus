#include "services/interfaces/workflow/fs2024/fs2024_step_params.hpp"

#include <cstdlib>

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
    if (it == step.parameters.end()) return fallback;
    if (it->second.type == WorkflowParameterValue::Type::Number) {
        return static_cast<float>(it->second.numberValue);
    }
    // A `${env:...}` value arrives as a string: take it when it is a
    // whole number, and fall back when it is empty or not one.
    if (it->second.type == WorkflowParameterValue::Type::String) {
        const std::string& text = it->second.stringValue;
        char* end = nullptr;
        const float value = std::strtof(text.c_str(), &end);
        if (!text.empty() && end && *end == '\0') return value;
    }
    return fallback;
}

}  // namespace sdl3cpp::services::impl
