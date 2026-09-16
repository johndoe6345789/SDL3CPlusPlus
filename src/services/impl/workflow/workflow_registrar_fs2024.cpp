#include "services/impl/workflow/workflow_registrar_categories.hpp"

#include "services/interfaces/workflow/fs2024/fs2024_player_steps.hpp"
#include "services/interfaces/workflow/fs2024/fs2024_terrain_draw_step.hpp"
#include "services/interfaces/workflow/fs2024/fs2024_terrain_free_step.hpp"
#include "services/interfaces/workflow/fs2024/fs2024_terrain_load_step.hpp"

#include <memory>

namespace sdl3cpp::services::impl::registrar_detail {

int RegisterFs2024Steps(std::shared_ptr<IWorkflowStepRegistry> registry,
                        std::shared_ptr<ILogger> logger) {
    if (!registry) return 0;
    // One ground shared by every fs2024 step; far too large for the
    // context, and Bullet reads its heights in place.
    auto state = std::make_shared<Fs2024TerrainState>();
    registry->RegisterStep(
        std::make_shared<WorkflowFs2024TerrainLoadStep>(logger, state));
    registry->RegisterStep(
        std::make_shared<WorkflowFs2024TerrainDrawStep>(logger, state));
    registry->RegisterStep(
        std::make_shared<WorkflowFs2024TerrainFreeStep>(logger, state));
    registry->RegisterStep(
        std::make_shared<WorkflowFs2024PlayerSpawnStep>(logger, state));
    registry->RegisterStep(
        std::make_shared<WorkflowFs2024GroundGuardStep>(logger, state));
    return 5;
}

}  // namespace sdl3cpp::services::impl::registrar_detail
