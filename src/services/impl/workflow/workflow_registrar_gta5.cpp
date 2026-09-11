#include "services/impl/workflow/workflow_registrar_categories.hpp"

#include "services/interfaces/workflow/gta5/gta5_stream_state.hpp"
#include "services/interfaces/workflow/gta5/gta5_lod_select_step.hpp"
#include "services/interfaces/workflow/gta5/gta5_tiles_draw_step.hpp"
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

    // The four steps share one streaming state. It holds the geometry cache
    // and the resident tile set, which are far too large to round-trip through
    // the workflow context every frame, so it is injected here instead.
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
        std::make_shared<WorkflowGta5TilesDrawStep>(logger, state));
    registry->RegisterStep(
        std::make_shared<WorkflowGta5VehicleSpawnStep>(logger, state));
    registry->RegisterStep(
        std::make_shared<WorkflowGta5VehiclesSyncStep>(logger, state));
    registry->RegisterStep(
        std::make_shared<WorkflowGta5VehicleControlStep>(logger, state));

    return 8;
}

}  // namespace sdl3cpp::services::impl::registrar_detail
