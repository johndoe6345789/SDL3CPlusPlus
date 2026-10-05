#include "services/interfaces/workflow/racer/racer_step_params.hpp"

#include <cstdlib>

namespace sdl3cpp::services::impl {

std::string RacerStringParam(const WorkflowStepDefinition& step,
                             const std::string& name, const char* envName,
                             const std::string& fallback) {
    const auto it = step.parameters.find(name);
    if (it != step.parameters.end() &&
        it->second.type == WorkflowParameterValue::Type::String &&
        !it->second.stringValue.empty()) {
        return it->second.stringValue;
    }
    if (envName != nullptr) {
        if (const char* value = std::getenv(envName)) {
            if (value[0] != '\0') return value;
        }
    }
    return fallback;
}

int RacerIntParam(const WorkflowStepDefinition& step,
                  const std::string& name, int fallback) {
    const auto it = step.parameters.find(name);
    if (it == step.parameters.end() ||
        it->second.type != WorkflowParameterValue::Type::Number) {
        return fallback;
    }
    return static_cast<int>(it->second.numberValue);
}

}  // namespace sdl3cpp::services::impl
