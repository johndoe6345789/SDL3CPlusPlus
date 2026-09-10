#pragma once

#include "services/interfaces/workflow_context.hpp"
#include "services/interfaces/workflow_step_definition.hpp"

#include <string>

namespace sdl3cpp::services::impl {

/// Reads control.condition.if_else's `condition` input from the context
/// (must be wired via `step.inputs` and resolve to a bool). Throws
/// std::runtime_error if the input isn't wired or isn't a bool.
bool ReadIfElseCondition(const WorkflowStepDefinition& step,
                         const WorkflowContext& context);

/// Reads the `true_branch`/`false_branch` plugin-id inputs (each defaults
/// to empty if not wired). Throws std::runtime_error if neither is wired.
struct IfElseBranchIds {
    std::string trueBranchId;
    std::string falseBranchId;
};
IfElseBranchIds ReadIfElseBranchIds(const WorkflowStepDefinition& step);

}  // namespace sdl3cpp::services::impl
