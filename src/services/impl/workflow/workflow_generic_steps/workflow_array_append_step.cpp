#include "services/interfaces/workflow/workflow_generic_steps/workflow_array_append_step.hpp"

#include <stdexcept>
#include <utility>
#include <vector>

namespace sdl3cpp::services::impl {

WorkflowArrayAppendStep::WorkflowArrayAppendStep(std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)) {
}

std::string WorkflowArrayAppendStep::GetPluginId() const {
    return "array.append";
}

void WorkflowArrayAppendStep::Execute(const WorkflowStepDefinition& step,
                                      WorkflowContext& context) {
    // Get array key
    const auto arrayIt = step.inputs.find("array");
    if (arrayIt == step.inputs.end()) {
        throw std::runtime_error("array.append requires 'array' input");
    }
    const std::string& arrayKey = arrayIt->second;

    // Get value key to append
    const auto valueIt = step.inputs.find("value");
    if (valueIt == step.inputs.end()) {
        throw std::runtime_error("array.append requires 'value' input");
    }
    const std::string& valueKey = valueIt->second;

    // Get array from context
    const auto* arrayValue = context.TryGet<std::vector<std::any>>(arrayKey);
    if (!arrayValue) {
        throw std::runtime_error("array.append: array key '" + arrayKey + "' not found or not an array");
    }

    // Get value to append
    const auto* valueAny = context.TryGetAny(valueKey);
    if (!valueAny) {
        throw std::runtime_error("array.append: value key '" + valueKey + "' not found");
    }

    // Create a copy, append, and set back
    std::vector<std::any> modifiedArray = *arrayValue;
    modifiedArray.push_back(*valueAny);
    context.Set(arrayKey, modifiedArray);

    if (logger_) {
        logger_->Trace("WorkflowArrayAppendStep", "Execute",
                       "array=" + arrayKey + ", size=" + std::to_string(arrayValue->size()),
                       "Element appended to array");
    }
}

}  // namespace sdl3cpp::services::impl
