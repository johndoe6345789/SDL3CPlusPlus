#include "services/impl/workflow/workflow_registrar_categories.hpp"

#include "services/interfaces/workflow/workflow_media_catalog_scan_step.hpp"
#include "services/interfaces/workflow/workflow_media_item_select_step.hpp"

#include <memory>

namespace sdl3cpp::services::impl::registrar_detail {

int RegisterMediaSteps(std::shared_ptr<IWorkflowStepRegistry> registry,
                       std::shared_ptr<ILogger> logger,
                       std::shared_ptr<IConfigService> configSvc,
                       std::shared_ptr<IAudioService> audioSvc) {
    if (!registry) return 0;

    int count = 0;

    // ── Media (service-dependent, nullptr until wired) ────────
    registry->RegisterStep(
        std::make_shared<WorkflowMediaCatalogScanStep>(configSvc, logger));
    registry->RegisterStep(
        std::make_shared<WorkflowMediaItemSelectStep>(audioSvc, logger));
    count += 2;

    return count;
}

}  // namespace sdl3cpp::services::impl::registrar_detail
