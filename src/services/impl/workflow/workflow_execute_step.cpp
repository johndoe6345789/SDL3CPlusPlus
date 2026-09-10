#include "services/interfaces/workflow/workflow_execute_step.hpp"
#include "services/interfaces/workflow/workflow_execute_helpers.hpp"
#include "services/interfaces/workflow_definition.hpp"

namespace sdl3cpp::services::impl {

WorkflowExecuteStep::WorkflowExecuteStep(
    std::shared_ptr<ILogger> logger,
    std::shared_ptr<IWorkflowExecutor> executor)
    : logger_(std::move(logger)), executor_(std::move(executor)) {
    if (logger_) {
        logger_->Trace("WorkflowExecuteStep", "Constructor", "Entry");
    }
}

void WorkflowExecuteStep::Execute(const WorkflowStepDefinition& step,
                                  WorkflowContext& context) {
    if (logger_) {
        logger_->Trace("WorkflowExecuteStep", "Execute", "Entry");
    }

    auto package = step.parameters.find("package");
    auto workflow = step.parameters.find("workflow");
    if (package == step.parameters.end() ||
        workflow == step.parameters.end()) {
        if (logger_) {
            logger_->Warn("WorkflowExecuteStep::Execute: Missing 'package' "
                         "or 'workflow' parameter");
        }
        return;
    }

    std::string packageName = package->second.stringValue;
    std::string workflowName = workflow->second.stringValue;
    if (packageName.empty() || workflowName.empty()) {
        if (logger_) {
            logger_->Warn(
                "WorkflowExecuteStep::Execute: Empty package or workflow "
                "name");
        }
        return;
    }

    if (logger_) {
        logger_->Trace("WorkflowExecuteStep", "Execute",
                       "package=" + packageName + ", workflow=" + workflowName,
                       "Loading child workflow");
    }

    auto childWorkflow = LoadChildWorkflow(logger_, packageName, workflowName);

    // Execute it with the same context (context passes through).
    executor_->Execute(childWorkflow, context);

    if (logger_) {
        logger_->Trace("WorkflowExecuteStep", "Execute",
                       "workflow=" + workflowName,
                       "Child workflow execution complete");
    }
}

}  // namespace sdl3cpp::services::impl
