#include "services/impl/workflow/workflow_registrar_categories.hpp"

#include "services/interfaces/workflow/scene/workflow_scene_create_step.hpp"
#include "services/interfaces/workflow/scene/workflow_scene_load_step.hpp"
#include "services/interfaces/workflow/scene/workflow_scene_update_step.hpp"
#include "services/interfaces/workflow/scene/workflow_scene_clear_step.hpp"
#include "services/interfaces/workflow/scene/workflow_scene_set_active_step.hpp"
#include "services/interfaces/workflow/scene/workflow_scene_add_geometry_step.hpp"
#include "services/interfaces/workflow/scene/workflow_scene_remove_geometry_step.hpp"
#include "services/interfaces/workflow/scene/workflow_scene_get_bounds_step.hpp"

#include <memory>

namespace sdl3cpp::services::impl::registrar_detail {

int RegisterSceneSteps(
    std::shared_ptr<IWorkflowStepRegistry> registry,
    std::shared_ptr<ILogger> logger,
    std::shared_ptr<ISceneService> sceneSvc) {
    if (!registry) return 0;

    int count = 0;

    // ── Scene (service-dependent, nullptr until wired) ─────────
    registry->RegisterStep(
        std::make_shared<WorkflowSceneCreateStep>(sceneSvc, logger));
    registry->RegisterStep(
        std::make_shared<WorkflowSceneLoadStep>(sceneSvc, logger));
    registry->RegisterStep(
        std::make_shared<WorkflowSceneUpdateStep>(sceneSvc, logger));
    registry->RegisterStep(
        std::make_shared<WorkflowSceneClearStep>(sceneSvc, logger));
    registry->RegisterStep(
        std::make_shared<WorkflowSceneSetActiveStep>(sceneSvc, logger));
    registry->RegisterStep(
        std::make_shared<WorkflowSceneAddGeometryStep>(sceneSvc, logger));
    registry->RegisterStep(
        std::make_shared<WorkflowSceneRemoveGeometryStep>(sceneSvc, logger));
    registry->RegisterStep(
        std::make_shared<WorkflowSceneGetBoundsStep>(sceneSvc, logger));
    count += 8;

    return count;
}

}  // namespace sdl3cpp::services::impl::registrar_detail
