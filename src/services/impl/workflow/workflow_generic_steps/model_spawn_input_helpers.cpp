#include "services/interfaces/workflow/workflow_generic_steps/model_spawn_input_helpers.hpp"
#include "services/interfaces/workflow/workflow_generic_steps/model_input_helpers.hpp"

#include <stdexcept>

namespace sdl3cpp::services::impl {

std::array<float, 16> ReadMatrix(
    const WorkflowStepDefinition& step, const WorkflowContext& context,
    const WorkflowStepParameterResolver& parameterResolver) {
    static constexpr std::array<float, 16> kIdentity = {
        1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f};
    return ReadMatrixInput(step, context, parameterResolver, "model.spawn",
                           &kIdentity);
}

std::vector<std::string> ReadShaderKeys(
    const WorkflowStepDefinition& step, const WorkflowContext& context,
    const WorkflowStepParameterResolver& parameterResolver) {
    auto it = step.inputs.find("shader_keys");
    if (it != step.inputs.end()) {
        const auto* list =
            context.TryGet<std::vector<std::string>>(it->second);
        if (!list) {
            throw std::runtime_error(
                "model.spawn requires shader_keys list input");
        }
        return *list;
    }
    auto singleIt = step.inputs.find("shader_key");
    if (singleIt != step.inputs.end()) {
        const auto* value = context.TryGet<std::string>(singleIt->second);
        if (!value) {
            throw std::runtime_error(
                "model.spawn requires shader_key string input");
        }
        return {*value};
    }
    if (const auto* param =
            parameterResolver.FindParameter(step, "shader_keys")) {
        if (param->type != WorkflowParameterValue::Type::StringList) {
            throw std::runtime_error(
                "model.spawn parameter 'shader_keys' must be string list");
        }
        return param->stringList;
    }
    if (const auto* param =
            parameterResolver.FindParameter(step, "shader_key")) {
        if (param->type != WorkflowParameterValue::Type::String) {
            throw std::runtime_error(
                "model.spawn parameter 'shader_key' must be string");
        }
        return {param->stringValue};
    }
    throw std::runtime_error("model.spawn requires shader_key(s)");
}

std::string ReadObjectType(
    const WorkflowStepDefinition& step, const WorkflowContext& context,
    const WorkflowStepParameterResolver& parameterResolver) {
    static const std::string kEmpty;
    return ReadObjectTypeInput(step, context, parameterResolver,
                               "model.spawn", &kEmpty);
}

}  // namespace sdl3cpp::services::impl
