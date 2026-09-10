#include "services/interfaces/workflow/workflow_generic_steps/workflow_control_if_else_step.hpp"
#include "services/interfaces/workflow/workflow_generic_steps/control_if_else_helpers.hpp"

#include <stdexcept>

namespace sdl3cpp::services::impl {

WorkflowControlIfElseStep::WorkflowControlIfElseStep(
    std::shared_ptr<ILogger> logger,
    std::shared_ptr<IWorkflowStepRegistry> registry)
    : logger_(std::move(logger)), registry_(std::move(registry)) {
    if (!registry_) {
        throw std::runtime_error(
            "WorkflowControlIfElseStep requires a step registry");
    }
}

std::string WorkflowControlIfElseStep::GetPluginId() const {
    return "control.condition.if_else";
}

void WorkflowControlIfElseStep::Execute(const WorkflowStepDefinition& step,
                                        WorkflowContext& context) {
    const bool condition           = ReadIfElseCondition(step, context);
    const IfElseBranchIds branches = ReadIfElseBranchIds(step);
    const std::string& selectedBranchId =
        condition ? branches.trueBranchId : branches.falseBranchId;

    if (selectedBranchId.empty()) {
        if (logger_) {
            logger_->Trace(
                "WorkflowControlIfElseStep", "Execute",
                "condition=" + std::string(condition ? "true" : "false"),
                "No branch to execute");
        }
        return;
    }

    auto branchHandler = registry_->GetStep(selectedBranchId);
    if (!branchHandler) {
        throw std::runtime_error("control.condition.if_else: branch step '" +
                                 selectedBranchId + "' not found");
    }

    // Create minimal step definition for the branch.
    WorkflowStepDefinition branchStep;
    branchStep.plugin = selectedBranchId;
    branchStep.id     = selectedBranchId;
    branchHandler->Execute(branchStep, context);

    if (logger_) {
        logger_->Trace(
            "WorkflowControlIfElseStep", "Execute",
            "condition=" + std::string(condition ? "true" : "false") +
                ", branch=" + selectedBranchId,
            "Executed control flow branch");
    }
}

}  // namespace sdl3cpp::services::impl
