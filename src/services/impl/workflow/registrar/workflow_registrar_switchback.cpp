#include "services/impl/workflow/registrar/workflow_registrar_switchback.hpp"

#include "services/interfaces/workflow/switchback/camera/switchback_camera_lens_step.hpp"
#include "services/interfaces/workflow/switchback/checkpoint/switchback_checkpoint_arrow_step.hpp"
#include "services/interfaces/workflow/switchback/dash/switchback_dash_step.hpp"
#include "services/interfaces/workflow/switchback/race/switchback_race_restart_step.hpp"
#include "services/interfaces/workflow/switchback/terrain/switchback_terrain_draw_step.hpp"
#include "services/interfaces/workflow/switchback/terrain/switchback_terrain_load_step.hpp"
#include "services/interfaces/workflow/switchback/terrain/switchback_terrain_state.hpp"
#include "services/interfaces/workflow/switchback/vehicle/switchback_car_control_step.hpp"

namespace sdl3cpp::services::impl::registrar_detail {

int RegisterSwitchbackSteps(std::shared_ptr<IWorkflowStepRegistry> registry,
                            std::shared_ptr<ILogger> logger,
                            std::shared_ptr<Gta5StreamState> vehicles) {
    if (!registry) return 0;
    auto terrain = std::make_shared<SwitchbackTerrainState>();
    registry->RegisterStep(
        std::make_shared<WorkflowSwitchbackTerrainLoadStep>(logger, terrain));
    registry->RegisterStep(
        std::make_shared<WorkflowSwitchbackTerrainDrawStep>(logger, terrain));
    registry->RegisterStep(
        std::make_shared<WorkflowSwitchbackRaceRestartStep>(logger, vehicles));
    registry->RegisterStep(
        std::make_shared<WorkflowSwitchbackCarControlStep>(logger, vehicles));
    registry->RegisterStep(
        std::make_shared<WorkflowSwitchbackCheckpointArrowStep>(logger,
                                                                vehicles,
                                                                terrain));
    registry->RegisterStep(
        std::make_shared<WorkflowSwitchbackDashStep>(logger, vehicles));
    registry->RegisterStep(
        std::make_shared<WorkflowSwitchbackCameraLensStep>(logger));
    return 7;
}

}  // namespace sdl3cpp::services::impl::registrar_detail
