#include "services/interfaces/workflow/workflow_exit_step.hpp"
#include "services/interfaces/workflow_step_definition.hpp"
#include "services/interfaces/workflow_parameter_value.hpp"
#include <cstdlib>

namespace sdl3cpp::services::impl {

WorkflowExitStep::WorkflowExitStep(std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)) {}

std::string WorkflowExitStep::GetPluginId() const {
    return "system.exit";
}

void WorkflowExitStep::Execute(const WorkflowStepDefinition& step,
                               WorkflowContext& context) {
    if (logger_) {
        logger_->Trace("WorkflowExitStep", "Execute", "", "Entry");
    }

    // Get parameters
    auto getIntParam = [&](const std::string& name, int defaultValue) -> int {
        auto it = step.parameters.find(name);
        if (it != step.parameters.end() && it->second.type == WorkflowParameterValue::Type::Number) {
            return static_cast<int>(it->second.numberValue);
        }
        return defaultValue;
    };

    auto getStringParam = [&](const std::string& name, const std::string& defaultValue) -> std::string {
        auto it = step.parameters.find(name);
        if (it != step.parameters.end()) {
            return it->second.stringValue;
        }
        return defaultValue;
    };

    // Support multiple exit codes with conditions
    // Parameters:
    //   status_code: default exit code (default: 0)
    //   condition: context key to check (e.g., "screenshot_captured")
    //   code_on_true: exit code if condition is true
    //   code_on_false: exit code if condition is false
    //   message: optional message to log before exit

    int defaultStatusCode = getIntParam("status_code", 0);
    std::string message = getStringParam("message", "");
    std::string condition = getStringParam("condition", "");
    int codeOnTrue = getIntParam("code_on_true", 0);
    int codeOnFalse = getIntParam("code_on_false", 1);

    int statusCode = defaultStatusCode;

    // If condition is specified, evaluate it
    if (!condition.empty()) {
        bool conditionMet = context.Get<bool>(condition, false);
        statusCode = conditionMet ? codeOnTrue : codeOnFalse;

        if (logger_) {
            logger_->Info("WorkflowExitStep: Condition '" + condition + "' = " +
                         (conditionMet ? "true" : "false") + ", exit_code=" + std::to_string(statusCode));
        }
    }

    if (!message.empty() && logger_) {
        logger_->Info("WorkflowExitStep: " + message);
    }

    if (logger_) {
        logger_->Info("WorkflowExitStep: Exiting with code " + std::to_string(statusCode));
    }

    // Exit with the determined status code
    std::exit(statusCode);
}

}  // namespace sdl3cpp::services::impl
