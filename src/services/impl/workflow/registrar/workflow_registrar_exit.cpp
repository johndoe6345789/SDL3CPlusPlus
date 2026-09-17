#include "services/impl/workflow/registrar/workflow_registrar_categories.hpp"

#include "services/interfaces/workflow/system/workflow_exit_step.hpp"

#include <memory>

namespace sdl3cpp::services::impl::registrar_detail {

int RegisterExitStep(std::shared_ptr<IWorkflowStepRegistry> registry,
                     std::shared_ptr<ILogger> logger) {
    if (!registry) return 0;

    int count = 0;

    // ── System ─────────────────────────────────────────────────
    registry->RegisterStep(std::make_shared<WorkflowExitStep>(logger));
    count += 1;

    return count;
}

}  // namespace sdl3cpp::services::impl::registrar_detail
