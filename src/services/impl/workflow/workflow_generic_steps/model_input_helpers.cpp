#include "services/interfaces/workflow/workflow_generic_steps/model_input_helpers.hpp"

#include <stdexcept>
#include <vector>

namespace sdl3cpp::services::impl {

std::array<float, 16> ReadMatrixInput(
    const WorkflowStepDefinition& step, const WorkflowContext& context,
    const WorkflowStepParameterResolver& parameterResolver,
    const std::string& stepName, const std::array<float, 16>* fallback) {
    auto it = step.inputs.find("matrix");
    if (it != step.inputs.end()) {
        const auto* list = context.TryGet<std::vector<double>>(it->second);
        if (!list || list->size() != 16u) {
            throw std::runtime_error(stepName +
                                     " requires matrix list of 16 numbers");
        }
        std::array<float, 16> matrix{};
        for (size_t i = 0; i < 16; ++i) {
            matrix[i] = static_cast<float>((*list)[i]);
        }
        return matrix;
    }
    if (const auto* param = parameterResolver.FindParameter(step, "matrix")) {
        if (param->type != WorkflowParameterValue::Type::NumberList ||
            param->numberList.size() != 16u) {
            throw std::runtime_error(
                stepName + " parameter 'matrix' must be number list of 16");
        }
        std::array<float, 16> matrix{};
        for (size_t i = 0; i < 16; ++i) {
            matrix[i] = static_cast<float>(param->numberList[i]);
        }
        return matrix;
    }
    if (fallback) return *fallback;
    throw std::runtime_error(stepName + " requires matrix input");
}

std::string ReadObjectTypeInput(
    const WorkflowStepDefinition& step, const WorkflowContext& context,
    const WorkflowStepParameterResolver& parameterResolver,
    const std::string& stepName, const std::string* fallback) {
    auto it = step.inputs.find("object_type");
    if (it != step.inputs.end()) {
        const auto* value = context.TryGet<std::string>(it->second);
        if (!value) {
            throw std::runtime_error(stepName +
                                     " requires object_type string input");
        }
        return *value;
    }
    if (const auto* param =
            parameterResolver.FindParameter(step, "object_type")) {
        if (param->type != WorkflowParameterValue::Type::String) {
            throw std::runtime_error(stepName +
                                     " parameter 'object_type' must be string");
        }
        return param->stringValue;
    }
    if (fallback) return *fallback;
    throw std::runtime_error(stepName + " requires object_type");
}

}  // namespace sdl3cpp::services::impl
