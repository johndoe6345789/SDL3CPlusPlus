#include "services/interfaces/workflow/workflow_generic_steps/control_if_else_helpers.hpp"

#include <stdexcept>

namespace sdl3cpp::services::impl {

bool ReadIfElseCondition(const WorkflowStepDefinition& step,
                         const WorkflowContext& context) {
    const auto conditionIt = step.inputs.find("condition");
    if (conditionIt == step.inputs.end()) {
        throw std::runtime_error(
            "control.condition.if_else requires 'condition' input");
    }

    const std::string& conditionKey = conditionIt->second;
    const auto* conditionValue = context.TryGet<bool>(conditionKey);
    if (!conditionValue) {
        throw std::runtime_error(
            "control.condition.if_else: condition input '" + conditionKey +
            "' must be bool");
    }
    return *conditionValue;
}

IfElseBranchIds ReadIfElseBranchIds(const WorkflowStepDefinition& step) {
    IfElseBranchIds ids;

    const auto trueBranchIt = step.inputs.find("true_branch");
    if (trueBranchIt != step.inputs.end()) {
        ids.trueBranchId = trueBranchIt->second;
    }

    const auto falseBranchIt = step.inputs.find("false_branch");
    if (falseBranchIt != step.inputs.end()) {
        ids.falseBranchId = falseBranchIt->second;
    }

    if (ids.trueBranchId.empty() && ids.falseBranchId.empty()) {
        throw std::runtime_error(
            "control.condition.if_else requires at least 'true_branch' or "
            "'false_branch'");
    }
    return ids;
}

}  // namespace sdl3cpp::services::impl
