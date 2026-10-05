#include "services/impl/workflow/registrar/workflow_registrar_categories.hpp"

#include "services/interfaces/workflow/racer/racer_assets_step.hpp"

#include <memory>

namespace sdl3cpp::services::impl::registrar_detail {

int RegisterRacerSteps(std::shared_ptr<IWorkflowStepRegistry> registry,
                       std::shared_ptr<ILogger> logger) {
    if (!registry) return 0;
    registry->RegisterStep(
        std::make_shared<WorkflowRacerAssetsStep>(logger));
    return 1;
}

}  // namespace sdl3cpp::services::impl::registrar_detail
