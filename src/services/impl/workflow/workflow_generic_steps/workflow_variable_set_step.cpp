#include "services/interfaces/workflow/workflow_generic_steps/workflow_variable_set_step.hpp"

#include <stdexcept>
#include <utility>

namespace sdl3cpp::services::impl {

WorkflowVariableSetStep::WorkflowVariableSetStep(std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)) {
}

std::string WorkflowVariableSetStep::GetPluginId() const {
    return "control.variable.set";
}

void WorkflowVariableSetStep::Execute(const WorkflowStepDefinition& step,
                                      WorkflowContext& context) {
    // Get variable name
    const auto nameIt = step.inputs.find("name");
    if (nameIt == step.inputs.end()) {
        throw std::runtime_error("control.variable.set requires 'name' input");
    }
    const std::string& varName = nameIt->second;

    if (varName.empty()) {
        throw std::runtime_error("control.variable.set: variable name cannot be empty");
    }

    // Get value from context
    const auto valueIt = step.inputs.find("value");
    if (valueIt == step.inputs.end()) {
        throw std::runtime_error("control.variable.set: 'value' input required for variable '" + varName + "'");
    }
    const std::string& valueKey = valueIt->second;

    // Look up the value in context (could be from context or a literal)
    const auto* anyValue = context.TryGetAny(valueKey);
    if (!anyValue) {
        throw std::runtime_error("control.variable.set: value key '" + valueKey + "' not found in context");
    }

    // Store in context with var.{name} prefix
    const std::string fullKey = "var." + varName;
    context.Set(fullKey, *anyValue);

    if (logger_) {
        logger_->Trace("WorkflowVariableSetStep", "Execute",
                       "name=" + varName + ", valueKey=" + valueKey,
                       "Variable set successfully");
    }
}

}  // namespace sdl3cpp::services::impl
