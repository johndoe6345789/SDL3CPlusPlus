#include "services/impl/workflow/workflow_registrar_categories.hpp"

#include "services/interfaces/workflow/workflow_generic_steps/workflow_physics_world_create_step.hpp"
#include "services/interfaces/workflow/workflow_generic_steps/workflow_physics_body_add_step.hpp"
#include "services/interfaces/workflow/workflow_generic_steps/workflow_physics_step_step.hpp"
#include "services/interfaces/workflow/workflow_generic_steps/workflow_physics_fps_move_step.hpp"
#include "services/interfaces/workflow/workflow_generic_steps/workflow_physics_sync_transforms_step.hpp"

#include <memory>

namespace sdl3cpp::services::impl::registrar_detail {

int RegisterPhysicsSteps(std::shared_ptr<IWorkflowStepRegistry> registry,
                         std::shared_ptr<ILogger> logger) {
    if (!registry) return 0;

    int count = 0;

    // ── Physics ────────────────────────────────────────────────
    registry->RegisterStep(
        std::make_shared<WorkflowPhysicsWorldCreateStep>(logger));
    registry->RegisterStep(
        std::make_shared<WorkflowPhysicsBodyAddStep>(logger));
    registry->RegisterStep(std::make_shared<WorkflowPhysicsStepStep>(logger));
    registry->RegisterStep(
        std::make_shared<WorkflowPhysicsFpsMoveStep>(logger));
    registry->RegisterStep(
        std::make_shared<WorkflowPhysicsSyncTransformsStep>(logger));
    count += 5;

    return count;
}

}  // namespace sdl3cpp::services::impl::registrar_detail
