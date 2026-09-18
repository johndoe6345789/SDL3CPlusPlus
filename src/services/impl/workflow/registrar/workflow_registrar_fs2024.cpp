#include "services/impl/workflow/registrar/workflow_registrar_categories.hpp"

#include "services/interfaces/workflow/fs2024/player/fs2024_player_cruise_step.hpp"
#include "services/interfaces/workflow/fs2024/player/fs2024_player_steps.hpp"
#include "services/interfaces/workflow/fs2024/terrain/fs2024_terrain_draw_step.hpp"
#include "services/interfaces/workflow/fs2024/terrain/fs2024_vectors_draw_step.hpp"
#include "services/interfaces/workflow/fs2024/terrain/fs2024_vegetation_draw_step.hpp"
#include "services/interfaces/workflow/fs2024/tiles/fs2024_tiles_evict_step.hpp"
#include "services/interfaces/workflow/fs2024/tiles/fs2024_tiles_load_step.hpp"
#include "services/interfaces/workflow/fs2024/tiles/fs2024_tiles_resolve_step.hpp"
#include "services/interfaces/workflow/fs2024/world/fs2024_world_open_step.hpp"
#include "services/interfaces/workflow/fs2024/world/fs2024_world_rebase_step.hpp"

#include <memory>

namespace sdl3cpp::services::impl::registrar_detail {

int RegisterFs2024Steps(std::shared_ptr<IWorkflowStepRegistry> registry,
                        std::shared_ptr<ILogger> logger) {
    if (!registry) return 0;
    // Every resident tile shared by every fs2024 step; far too large
    // for the context, and Bullet reads its heights in place.
    auto state = std::make_shared<Fs2024TileStreamState>();
    registry->RegisterStep(
        std::make_shared<WorkflowFs2024WorldOpenStep>(logger, state));
    registry->RegisterStep(
        std::make_shared<WorkflowFs2024WorldRebaseStep>(logger, state));
    registry->RegisterStep(
        std::make_shared<WorkflowFs2024TilesResolveStep>(logger, state));
    registry->RegisterStep(
        std::make_shared<WorkflowFs2024TilesLoadStep>(logger, state));
    registry->RegisterStep(
        std::make_shared<WorkflowFs2024TilesEvictStep>(logger, state));
    registry->RegisterStep(
        std::make_shared<WorkflowFs2024TilesFreeStep>(logger, state));
    registry->RegisterStep(
        std::make_shared<WorkflowFs2024TerrainDrawStep>(logger, state));
    registry->RegisterStep(
        std::make_shared<WorkflowFs2024PlayerSpawnStep>(logger, state));
    registry->RegisterStep(
        std::make_shared<WorkflowFs2024GroundGuardStep>(logger, state));
    registry->RegisterStep(
        std::make_shared<WorkflowFs2024PlayerCruiseStep>(logger, state));
    registry->RegisterStep(
        std::make_shared<WorkflowFs2024VectorsDrawStep>(logger, state));
    registry->RegisterStep(
        std::make_shared<WorkflowFs2024VegetationDrawStep>(logger, state));
    return 12;
}

}  // namespace sdl3cpp::services::impl::registrar_detail
