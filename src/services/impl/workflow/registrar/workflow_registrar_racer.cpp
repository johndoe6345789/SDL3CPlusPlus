#include "services/impl/workflow/registrar/workflow_registrar_categories.hpp"

#include "services/interfaces/workflow/racer/player/racer_lap_timer_step.hpp"
#include "services/interfaces/workflow/racer/player/racer_pod_drive_step.hpp"
#include "services/interfaces/workflow/racer/racer_assets_step.hpp"
#include "services/interfaces/workflow/racer/render/racer_chase_cam_step.hpp"
#include "services/interfaces/workflow/racer/render/racer_hud_step.hpp"
#include "services/interfaces/workflow/racer/render/racer_scene_draw_step.hpp"
#include "services/interfaces/workflow/racer/world/racer_world_load_step.hpp"

#include <memory>

namespace sdl3cpp::services::impl::registrar_detail {

int RegisterRacerSteps(std::shared_ptr<IWorkflowStepRegistry> registry,
                       std::shared_ptr<ILogger> logger) {
    if (!registry) return 0;
    // One loaded race shared by every racer.* step: it owns the GPU
    // buffers and textures the draw step reads, and the pod's state.
    auto state = std::make_shared<RacerWorldState>();
    registry->RegisterStep(std::make_shared<WorkflowRacerAssetsStep>(logger));
    registry->RegisterStep(
        std::make_shared<WorkflowRacerWorldLoadStep>(logger, state));
    registry->RegisterStep(
        std::make_shared<WorkflowRacerPodDriveStep>(logger, state));
    registry->RegisterStep(
        std::make_shared<WorkflowRacerLapTimerStep>(logger, state));
    registry->RegisterStep(
        std::make_shared<WorkflowRacerChaseCameraStep>(logger, state));
    registry->RegisterStep(
        std::make_shared<WorkflowRacerSceneDrawStep>(logger, state));
    registry->RegisterStep(
        std::make_shared<WorkflowRacerHudStep>(logger, state));
    return 7;
}

}  // namespace sdl3cpp::services::impl::registrar_detail
