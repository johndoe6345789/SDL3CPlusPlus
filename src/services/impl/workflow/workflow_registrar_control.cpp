#include "services/impl/workflow/workflow_registrar_categories.hpp"

#include "services/interfaces/workflow/workflow_generic_steps/workflow_control_for_each_step.hpp"
#include "services/interfaces/workflow/workflow_generic_steps/workflow_control_if_else_step.hpp"
#include "services/interfaces/workflow/workflow_generic_steps/workflow_control_switch_step.hpp"
#include "services/interfaces/workflow/workflow_generic_steps/workflow_try_catch_step.hpp"

#include <memory>

namespace sdl3cpp::services::impl::registrar_detail {

int RegisterControlSteps(std::shared_ptr<IWorkflowStepRegistry> registry,
                         std::shared_ptr<ILogger> logger) {
    if (!registry) return 0;

    int count = 0;

    // ── Control structures (need registry) ─────────────────────
    registry->RegisterStep(
        std::make_shared<WorkflowControlForEachStep>(logger, registry));
    registry->RegisterStep(
        std::make_shared<WorkflowControlIfElseStep>(logger, registry));
    registry->RegisterStep(
        std::make_shared<WorkflowControlSwitchStep>(logger, registry));
    registry->RegisterStep(
        std::make_shared<WorkflowTryCatchStep>(logger, registry));
    count += 4;

    return count;
}

}  // namespace sdl3cpp::services::impl::registrar_detail
