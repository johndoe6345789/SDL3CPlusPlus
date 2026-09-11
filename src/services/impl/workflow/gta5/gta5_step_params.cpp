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

float Gta5NumberOr(const WorkflowStepDefinition& step,
                   const std::string& name, float fallback) {
    const auto it = step.parameters.find(name);
    if (it == step.parameters.end() ||
        it->second.type != WorkflowParameterValue::Type::Number) {
        return fallback;
    }
    return static_cast<float>(it->second.numberValue);
}

std::string Gta5ResolvePath(const WorkflowStepDefinition& step,
                            const WorkflowContext& context,
                            const std::string& name,
                            const std::string& fallback) {
    const std::string relative = Gta5ParameterOr(step, name, fallback);
    const std::string packageDir =
        context.Get<std::string>("package_dir", "");
    return packageDir.empty() ? relative : packageDir + "/" + relative;
}

}  // namespace sdl3cpp::services::impl
