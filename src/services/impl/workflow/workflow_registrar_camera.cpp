#include "services/impl/workflow/workflow_registrar_categories.hpp"

#include "services/interfaces/workflow/workflow_generic_steps/workflow_camera_setup_step.hpp"
#include "services/interfaces/workflow/workflow_generic_steps/workflow_camera_fps_update_step.hpp"
#include "services/interfaces/workflow/workflow_generic_steps/workflow_camera_look_at_step.hpp"
#include "services/interfaces/workflow/workflow_generic_steps/workflow_camera_set_fov_step.hpp"
#include "services/interfaces/workflow/workflow_generic_steps/workflow_camera_set_pose_step.hpp"
#include "services/interfaces/workflow/workflow_generic_steps/workflow_camera_teleport_step.hpp"

#include <memory>

namespace sdl3cpp::services::impl::registrar_detail {

int RegisterCameraSteps(
    std::shared_ptr<IWorkflowStepRegistry> registry,
    std::shared_ptr<ILogger> logger) {
    if (!registry) return 0;

    int count = 0;

    // ── Camera ─────────────────────────────────────────────────
    registry->RegisterStep(std::make_shared<WorkflowCameraSetupStep>(logger));
    registry->RegisterStep(
        std::make_shared<WorkflowCameraFpsUpdateStep>(logger));
    registry->RegisterStep(std::make_shared<WorkflowCameraLookAtStep>(logger));
    registry->RegisterStep(std::make_shared<WorkflowCameraSetFovStep>(logger));
    registry->RegisterStep(
        std::make_shared<WorkflowCameraSetPoseStep>(logger));
    registry->RegisterStep(
        std::make_shared<WorkflowCameraTeleportStep>(logger));
    count += 6;

    return count;
}

}  // namespace sdl3cpp::services::impl::registrar_detail
