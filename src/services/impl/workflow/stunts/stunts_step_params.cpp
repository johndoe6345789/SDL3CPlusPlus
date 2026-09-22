#include "services/interfaces/workflow/stunts/stunts_step_params.hpp"

#include <cstdlib>

namespace sdl3cpp::services::impl {

std::string StuntsStringOr(const WorkflowStepDefinition& step,
                           const std::string& name, const char* envName,
                           const std::string& fallback) {
    const auto it = step.parameters.find(name);
    if (it != step.parameters.end() &&
        it->second.type == WorkflowParameterValue::Type::String &&
        !it->second.stringValue.empty()) {
        return it->second.stringValue;
    }
    // The launcher passes the install directory by environment, so a
    // workflow need not name a path that differs on every machine.
    if (envName != nullptr) {
        if (const char* value = std::getenv(envName)) {
            if (value[0] != '\0') return value;
        }
    }
    return fallback;
}

float StuntsNumberOr(const WorkflowStepDefinition& step,
                     const std::string& name, float fallback) {
    const auto it = step.parameters.find(name);
    if (it == step.parameters.end()) return fallback;
    if (it->second.type == WorkflowParameterValue::Type::Number) {
        return static_cast<float>(it->second.numberValue);
    }
    if (it->second.type == WorkflowParameterValue::Type::String) {
        const std::string& text = it->second.stringValue;
        char* end = nullptr;
        const float value = std::strtof(text.c_str(), &end);
        if (!text.empty() && end && *end == '\0') return value;
    }
    return fallback;
}

}  // namespace sdl3cpp::services::impl
