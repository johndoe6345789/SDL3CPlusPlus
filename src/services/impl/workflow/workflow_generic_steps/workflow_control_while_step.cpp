#include "services/interfaces/workflow/workflow_generic_steps/workflow_control_while_step.hpp"
#include "services/interfaces/workflow/workflow_generic_steps/workflow_package_loader.hpp"
#include "services/interfaces/workflow_context.hpp"
#include "services/interfaces/workflow_step_definition.hpp"

#include <stdexcept>
#include <utility>

namespace sdl3cpp::services::impl {

WorkflowControlWhileStep::WorkflowControlWhileStep(
    std::shared_ptr<ILogger> logger,
    std::shared_ptr<IWorkflowExecutor> executor)
    : logger_(std::move(logger)), executor_(std::move(executor)) {}

std::string WorkflowControlWhileStep::GetPluginId() const {
    return "control.loop.while";
}

void WorkflowControlWhileStep::Execute(const WorkflowStepDefinition& step,
                                       WorkflowContext& context) {
    const std::string conditionKey = RequireStringParam(
        step, "condition_key",
        "control.loop.while: Missing 'condition_key' parameter");
    const std::string packageName = RequireStringParam(
        step, "package",
        "control.loop.while: Missing 'package' or 'workflow' parameter");
    const std::string workflowName = RequireStringParam(
        step, "workflow",
        "control.loop.while: Missing 'package' or 'workflow' parameter");
    uint32_t maxIterations = 0;  // 0 = unlimited
    auto maxIt = step.parameters.find("max_iterations");
    if (maxIt != step.parameters.end() &&
        maxIt->second.type == WorkflowParameterValue::Type::Number) {
        maxIterations = static_cast<uint32_t>(maxIt->second.numberValue);
    }
    auto childWorkflow =
        LoadPackageWorkflow(logger_, packageName, workflowName);
    if (childWorkflow.steps.empty()) {
        throw std::runtime_error(
            "control.loop.while: Could not load workflow '" + workflowName +
            "' from package '" + packageName + "'");
    }
    if (logger_) {
        logger_->Info(
            "control.loop.while: Looping on '" + conditionKey +
            "', workflow=" + workflowName +
            (maxIterations > 0 ? ", max=" + std::to_string(maxIterations)
                               : ""));
    }
    // A key that was never set reads as false, so the loop would exit
    // after zero iterations; warn so that is distinguishable from a
    // deliberate false.
    if (logger_ && !context.Contains(conditionKey)) {
        logger_->Warn("control.loop.while: condition '" + conditionKey +
                     "' is not set in the context; set it with a "
                     "value.literal step beforehand.");
    }
    // Suppress per-step logging inside the frame loop for performance
    context.Set<bool>("_in_frame_loop", true);
    uint32_t iteration = 0;
    while (context.GetBool(conditionKey, false)) {
        if (maxIterations > 0 && iteration >= maxIterations) {
            if (logger_) {
                logger_->Warn("control.loop.while: Hit max iterations (" +
                             std::to_string(maxIterations) + ")");
            }
            break;
        }
        context.Set<double>("loop.iteration", static_cast<double>(iteration));
        executor_->Execute(childWorkflow, context);
        iteration++;
    }
    if (logger_) {
        logger_->Info("control.loop.while: Completed after " +
                     std::to_string(iteration) + " iterations");
    }
}

}  // namespace sdl3cpp::services::impl
