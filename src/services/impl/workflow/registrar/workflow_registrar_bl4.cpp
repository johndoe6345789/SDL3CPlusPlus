#include "services/impl/workflow/registrar/workflow_registrar_categories.hpp"

#include "services/interfaces/workflow/bl4/bl4_loading_text_step.hpp"
#include "services/interfaces/workflow/bl4/bl4_model_draw_step.hpp"
#include "services/interfaces/workflow/bl4/bl4_player_steps.hpp"
#include "services/interfaces/workflow/bl4/bl4_tiles_evict_step.hpp"
#include "services/interfaces/workflow/bl4/bl4_tiles_load_step.hpp"
#include "services/interfaces/workflow/bl4/bl4_tiles_resolve_step.hpp"

#include <memory>

namespace sdl3cpp::services::impl::registrar_detail {

int RegisterBl4Steps(std::shared_ptr<IWorkflowStepRegistry> registry,
                     std::shared_ptr<ILogger> logger) {
    if (!registry) return 0;
    // Every resident tile's instances and the shared geometry cache,
    // shared by every bl4 step; far too large for the context.
    auto state = std::make_shared<Bl4TileStreamState>();
    registry->RegisterStep(std::make_shared<WorkflowBl4TilesResolveStep>(logger, state));
    registry->RegisterStep(std::make_shared<WorkflowBl4TilesLoadStep>(logger, state));
    registry->RegisterStep(std::make_shared<WorkflowBl4TilesEvictStep>(logger, state));
    registry->RegisterStep(std::make_shared<WorkflowBl4TilesFreeStep>(logger, state));
    registry->RegisterStep(std::make_shared<WorkflowBl4ModelDrawStep>(logger, state));
    registry->RegisterStep(std::make_shared<WorkflowBl4PlayerSpawnStep>(logger, state));
    registry->RegisterStep(std::make_shared<WorkflowBl4PlayerHoldStep>(logger, state));
    registry->RegisterStep(std::make_shared<WorkflowBl4LoadingTextStep>(logger));
    registry->RegisterStep(std::make_shared<WorkflowBl4CameraOrbitStep>(logger));
    registry->RegisterStep(std::make_shared<WorkflowBl4PlayerRespawnStep>(logger));
    return 10;
}

}  // namespace sdl3cpp::services::impl::registrar_detail
