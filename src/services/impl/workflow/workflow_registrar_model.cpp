#include "services/impl/workflow/workflow_registrar_categories.hpp"

#include "services/interfaces/workflow/workflow_generic_steps/workflow_model_despawn_step.hpp"
#include "services/interfaces/workflow/workflow_generic_steps/workflow_model_set_transform_step.hpp"

#include <memory>

namespace sdl3cpp::services::impl::registrar_detail {

int RegisterModelSteps(std::shared_ptr<IWorkflowStepRegistry> registry,
                       std::shared_ptr<ILogger> logger) {
    if (!registry) return 0;

    int count = 0;

    // ── Model ──────────────────────────────────────────────────
    registry->RegisterStep(std::make_shared<WorkflowModelDespawnStep>(logger));
    registry->RegisterStep(
        std::make_shared<WorkflowModelSetTransformStep>(logger));
    count += 2;

    return count;
}

}  // namespace sdl3cpp::services::impl::registrar_detail
