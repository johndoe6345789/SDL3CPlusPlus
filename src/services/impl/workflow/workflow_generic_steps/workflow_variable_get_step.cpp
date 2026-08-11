#include "services/interfaces/workflow/workflow_generic_steps/workflow_variable_get_step.hpp"

#include <stdexcept>
#include <utility>

namespace sdl3cpp::services::impl {

WorkflowVariableGetStep::WorkflowVariableGetStep(std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)) {
}

std::string WorkflowVariableGetStep::GetPluginId() const {
    return "control.variable.get";
}

void WorkflowVariableGetStep::Execute(const WorkflowStepDefinition& step,
                                      WorkflowContext& context) {
    // Get variable name
    const auto nameIt = step.inputs.find("name");
    if (nameIt == step.inputs.end()) {
        throw std::runtime_error("control.variable.get requires 'name' input");
    }
    const std::string& varName = nameIt->second;

    if (varName.empty()) {
        throw std::runtime_error("control.variable.get: variable name cannot be empty");
    }

    // Get output key where value should be stored
    const auto outputIt = step.inputs.find("output");
    if (outputIt == step.inputs.end()) {
        throw std::runtime_error("control.variable.get: 'output' input required");
    }
    const std::string& outputKey = outputIt->second;

    // Look up variable from context
    const std::string fullKey = "var." + varName;
    const auto* anyValue = context.TryGetAny(fullKey);
    if (!anyValue) {
        throw std::runtime_error("control.variable.get: variable '" + varName + "' not found");
    }

    // Store in context at output location
    context.Set(outputKey, *anyValue);

    if (logger_) {
        logger_->Trace("WorkflowVariableGetStep", "Execute",
                       "name=" + varName + ", output=" + outputKey,
                       "Variable retrieved successfully");
    }
}

}  // namespace sdl3cpp::services::impl
