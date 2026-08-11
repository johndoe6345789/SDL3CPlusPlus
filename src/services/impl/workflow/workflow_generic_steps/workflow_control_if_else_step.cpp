#include "services/interfaces/workflow/workflow_generic_steps/workflow_control_if_else_step.hpp"

#include "services/interfaces/workflow/workflow_step_io_resolver.hpp"

#include <stdexcept>
#include <utility>

namespace sdl3cpp::services::impl {

WorkflowControlIfElseStep::WorkflowControlIfElseStep(
    std::shared_ptr<ILogger> logger,
    std::shared_ptr<IWorkflowStepRegistry> registry)
    : logger_(std::move(logger)),
      registry_(std::move(registry)) {
    if (!registry_) {
        throw std::runtime_error("WorkflowControlIfElseStep requires a step registry");
    }
}

std::string WorkflowControlIfElseStep::GetPluginId() const {
    return "control.condition.if_else";
}

void WorkflowControlIfElseStep::Execute(const WorkflowStepDefinition& step,
                                        WorkflowContext& context) {
    WorkflowStepIoResolver resolver;

    // Get condition value from inputs or parameters
    const auto conditionIt = step.inputs.find("condition");
    bool condition = false;

    if (conditionIt != step.inputs.end()) {
        const std::string& conditionKey = conditionIt->second;
        const auto* conditionValue = context.TryGet<bool>(conditionKey);
        if (!conditionValue) {
            throw std::runtime_error(
                "control.condition.if_else: condition input '" + conditionKey + "' must be bool");
        }
        condition = *conditionValue;
    } else {
        throw std::runtime_error("control.condition.if_else requires 'condition' input");
    }

    // Get branch plugin IDs
    std::string trueBranchId;
    std::string falseBranchId;

    const auto trueBranchIt = step.inputs.find("true_branch");
    if (trueBranchIt != step.inputs.end()) {
        trueBranchId = trueBranchIt->second;
    }

    const auto falseBranchIt = step.inputs.find("false_branch");
    if (falseBranchIt != step.inputs.end()) {
        falseBranchId = falseBranchIt->second;
    }

    if (trueBranchId.empty() && falseBranchId.empty()) {
        throw std::runtime_error(
            "control.condition.if_else requires at least 'true_branch' or 'false_branch'");
    }

    // Select and execute branch
    const std::string& selectedBranchId = condition ? trueBranchId : falseBranchId;

    if (!selectedBranchId.empty()) {
        auto branchHandler = registry_->GetStep(selectedBranchId);
        if (!branchHandler) {
            throw std::runtime_error(
                "control.condition.if_else: branch step '" + selectedBranchId + "' not found");
        }

        // Create minimal step definition for the branch
        WorkflowStepDefinition branchStep;
        branchStep.plugin = selectedBranchId;
        branchStep.id = selectedBranchId;

        branchHandler->Execute(branchStep, context);

        if (logger_) {
            logger_->Trace(
                "WorkflowControlIfElseStep", "Execute",
                "condition=" + std::string(condition ? "true" : "false") +
                    ", branch=" + selectedBranchId,
                "Executed control flow branch");
        }
    } else if (logger_) {
        logger_->Trace("WorkflowControlIfElseStep", "Execute",
                       "condition=" + std::string(condition ? "true" : "false"),
                       "No branch to execute");
    }
}

}  // namespace sdl3cpp::services::impl
