#include "services/impl/workflow/workflow_registrar_gta5_player.hpp"

#include "services/interfaces/workflow/gta5/gta5_player_camera_step.hpp"
#include "services/interfaces/workflow/gta5/gta5_player_character_step.hpp"
#include "services/interfaces/workflow/gta5/gta5_player_fly_step.hpp"
#include "services/interfaces/workflow/gta5/gta5_shadow_step.hpp"

namespace sdl3cpp::services::impl::registrar_detail {

int RegisterGta5PlayerSteps(std::shared_ptr<IWorkflowStepRegistry> registry,
                            std::shared_ptr<ILogger> logger,
                            std::shared_ptr<Gta5StreamState> state) {
    if (!registry) return 0;
    registry->RegisterStep(
        std::make_shared<WorkflowGta5PlayerFlyStep>(logger, state));
    registry->RegisterStep(
        std::make_shared<WorkflowGta5PlayerCameraStep>(logger, state));
    registry->RegisterStep(
        std::make_shared<WorkflowGta5PlayerCharacterStep>(logger, state));
    registry->RegisterStep(
        std::make_shared<WorkflowGta5ShadowDrawStep>(logger, state));
    return 4;
}

}  // namespace sdl3cpp::services::impl::registrar_detail
