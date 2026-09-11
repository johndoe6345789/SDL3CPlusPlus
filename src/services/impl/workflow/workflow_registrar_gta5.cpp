#include "services/impl/workflow/workflow_registrar_categories.hpp"

#include "services/interfaces/workflow/gta5/gta5_stream_state.hpp"
#include "services/interfaces/workflow/gta5/gta5_assets_index_step.hpp"
#include "services/interfaces/workflow/gta5/gta5_frame_mark_step.hpp"
#include "services/interfaces/workflow/gta5/gta5_frame_stats_step.hpp"
#include "services/interfaces/workflow/gta5/gta5_loading_text_step.hpp"
#include "services/interfaces/workflow/gta5/gta5_player_hold_step.hpp"
#include "services/interfaces/workflow/gta5/gta5_lod_select_step.hpp"
#include "services/interfaces/workflow/gta5/gta5_map_step.hpp"
#include "services/interfaces/workflow/gta5/gta5_player_camera_step.hpp"
#include "services/interfaces/workflow/gta5/gta5_player_character_step.hpp"
#include "services/interfaces/workflow/gta5/gta5_player_fly_step.hpp"
#include "services/interfaces/workflow/gta5/gta5_sky_draw_step.hpp"
#include "services/interfaces/workflow/gta5/gta5_tiles_cull_step.hpp"
#include "services/interfaces/workflow/gta5/gta5_tiles_draw_step.hpp"
#include "services/interfaces/workflow/gta5/gta5_vehicle_camera_step.hpp"
#include "services/interfaces/workflow/gta5/gta5_vehicle_control_step.hpp"
#include "services/interfaces/workflow/gta5/gta5_vehicle_spawn_step.hpp"
#include "services/interfaces/workflow/gta5/gta5_vehicles_sync_step.hpp"
#include "services/interfaces/workflow/gta5/gta5_tiles_evict_step.hpp"
#include "services/interfaces/workflow/gta5/gta5_tiles_load_step.hpp"
#include "services/interfaces/workflow/gta5/gta5_tiles_resolve_step.hpp"

#include <memory>

namespace sdl3cpp::services::impl::registrar_detail {

int RegisterGta5StreamingSteps(std::shared_ptr<IWorkflowStepRegistry> registry,
                               std::shared_ptr<ILogger> logger) {
    if (!registry) return 0;

    // One shared streaming state, far too large to go through the context.
    auto state = std::make_shared<Gta5StreamState>();

    registry->RegisterStep(
        std::make_shared<WorkflowGta5TilesResolveStep>(logger, state));
    registry->RegisterStep(
        std::make_shared<WorkflowGta5TilesLoadStep>(logger, state));
    registry->RegisterStep(
        std::make_shared<WorkflowGta5TilesEvictStep>(logger, state));
    registry->RegisterStep(
        std::make_shared<WorkflowGta5LodSelectStep>(logger, state));
    registry->RegisterStep(
        std::make_shared<WorkflowGta5TilesCullStep>(logger, state));
    registry->RegisterStep(
        std::make_shared<WorkflowGta5TilesDrawStep>(logger, state));
    registry->RegisterStep(
        std::make_shared<WorkflowGta5VehicleSpawnStep>(logger, state));
    registry->RegisterStep(
        std::make_shared<WorkflowGta5VehiclesSyncStep>(logger, state));
    registry->RegisterStep(
        std::make_shared<WorkflowGta5VehicleControlStep>(logger, state));
    registry->RegisterStep(
        std::make_shared<WorkflowGta5VehicleCameraStep>(logger, state));
    registry->RegisterStep(
        std::make_shared<WorkflowGta5AssetsIndexStep>(logger, state));
    registry->RegisterStep(
        std::make_shared<WorkflowGta5PlayerHoldStep>(logger, state));
    registry->RegisterStep(
        std::make_shared<WorkflowGta5LoadingTextStep>(logger));
    registry->RegisterStep(
        std::make_shared<WorkflowGta5FrameStatsStep>(logger, state));
    registry->RegisterStep(
        std::make_shared<WorkflowGta5FrameMarkStep>(logger, state));
    registry->RegisterStep(
        std::make_shared<WorkflowGta5MapDrawStep>(logger, state));
    registry->RegisterStep(
        std::make_shared<WorkflowGta5PlayerFlyStep>(logger, state));
    registry->RegisterStep(
        std::make_shared<WorkflowGta5PlayerCameraStep>(logger, state));
    registry->RegisterStep(
        std::make_shared<WorkflowGta5PlayerCharacterStep>(logger, state));
    // The sky needs no streaming state: it is the camera and the sun.
    registry->RegisterStep(std::make_shared<WorkflowGta5SkyDrawStep>(logger));

    return 20;
}

}  // namespace sdl3cpp::services::impl::registrar_detail
