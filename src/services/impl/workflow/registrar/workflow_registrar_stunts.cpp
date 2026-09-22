#include "services/impl/workflow/registrar/workflow_registrar_categories.hpp"

#include "services/interfaces/workflow/stunts/player/stunts_car_drive_step.hpp"
#include "services/interfaces/workflow/stunts/player/stunts_lap_timer_step.hpp"
#include "services/interfaces/workflow/stunts/render/stunts_car_draw_step.hpp"
#include "services/interfaces/workflow/stunts/render/stunts_chase_cam_step.hpp"
#include "services/interfaces/workflow/stunts/render/stunts_dash_step.hpp"
#include "services/interfaces/workflow/stunts/render/stunts_track_draw_step.hpp"
#include "services/interfaces/workflow/stunts/world/stunts_world_load_step.hpp"

#include <memory>

namespace sdl3cpp::services::impl::registrar_detail {

int RegisterStuntsSteps(std::shared_ptr<IWorkflowStepRegistry> registry,
                        std::shared_ptr<ILogger> logger) {
    if (!registry) return 0;
    // One loaded track, shared by every stunts.* step: it owns the GPU
    // buffers the draw step reads every frame, and the grid the car is
    // driven over.
    auto state = std::make_shared<StuntsWorldState>();
    registry->RegisterStep(
        std::make_shared<WorkflowStuntsWorldLoadStep>(logger, state));
    registry->RegisterStep(
        std::make_shared<WorkflowStuntsCarDriveStep>(logger, state));
    registry->RegisterStep(
        std::make_shared<WorkflowStuntsLapTimerStep>(logger, state));
    registry->RegisterStep(
        std::make_shared<WorkflowStuntsTrackDrawStep>(logger, state));
    registry->RegisterStep(
        std::make_shared<WorkflowStuntsChaseCameraStep>(logger));
    registry->RegisterStep(
        std::make_shared<WorkflowStuntsCarDrawStep>(logger, state));
    registry->RegisterStep(
        std::make_shared<WorkflowStuntsDashboardDrawStep>(logger));
    return 7;
}

}  // namespace sdl3cpp::services::impl::registrar_detail
