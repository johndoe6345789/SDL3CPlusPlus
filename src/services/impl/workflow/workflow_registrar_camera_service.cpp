#include "services/impl/workflow/workflow_registrar_categories.hpp"

#include "services/interfaces/workflow/workflow_generic_steps/workflow_camera_build_view_state_step.hpp"

#include <memory>

namespace sdl3cpp::services::impl::registrar_detail {

int RegisterCameraServiceSteps(std::shared_ptr<IWorkflowStepRegistry> registry,
                               std::shared_ptr<ILogger> logger,
                               std::shared_ptr<IConfigService> configSvc) {
    if (!registry) return 0;

    int count = 0;

    // ── Camera (service-dependent, nullptr until wired) ────────
    registry->RegisterStep(
        std::make_shared<WorkflowCameraBuildViewStateStep>(configSvc, logger));
    count += 1;

    return count;
}

}  // namespace sdl3cpp::services::impl::registrar_detail
