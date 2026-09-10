#include "services/interfaces/workflow/workflow_generic_steps/try_catch_dispatch.hpp"
#include "services/interfaces/workflow_step_definition.hpp"

#include <stdexcept>

namespace sdl3cpp::services::impl {

void ExecuteRegisteredStep(IWorkflowStepRegistry& registry,
                           const std::string& stepId, WorkflowContext& context,
                           const char* notFoundContext) {
    auto handler = registry.GetStep(stepId);
    if (!handler) {
        throw std::runtime_error(std::string("control.try.catch: ") +
                                 notFoundContext + " step '" + stepId +
                                 "' not found");
    }

    WorkflowStepDefinition stepDef;
    stepDef.plugin = stepId;
    stepDef.id = stepId;
    handler->Execute(stepDef, context);
}

}  // namespace sdl3cpp::services::impl
