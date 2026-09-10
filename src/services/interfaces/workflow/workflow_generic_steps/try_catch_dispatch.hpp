#pragma once

#include "services/interfaces/i_workflow_step_registry.hpp"
#include "services/interfaces/workflow_context.hpp"

#include <string>

namespace sdl3cpp::services::impl {

/**
 * @brief Looks up `stepId` in `registry` and executes it with a minimal
 *        WorkflowStepDefinition (plugin == id == stepId).
 *
 * Used by control.try.catch to run both its "try_step" and "catch_step"
 * the same way.
 *
 * @throws std::runtime_error if `stepId` is not registered; propagates
 *         whatever the step itself throws.
 */
void ExecuteRegisteredStep(IWorkflowStepRegistry& registry,
                           const std::string& stepId, WorkflowContext& context,
                           const char* notFoundContext);

}  // namespace sdl3cpp::services::impl
