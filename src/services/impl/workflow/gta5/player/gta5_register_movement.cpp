#include "services/interfaces/workflow/gta5/player/gta5_register_movement.hpp"

#include "services/interfaces/workflow/gta5/player/gta5_player_climb_step.hpp"
#include "services/interfaces/workflow/gta5/player/gta5_player_fly_step.hpp"
#include "services/interfaces/workflow/gta5/player/gta5_player_swim_step.hpp"

namespace sdl3cpp::services::impl {

void RegisterGta5MovementSteps(
    IWorkflowStepRegistry& registry, const std::shared_ptr<ILogger>& logger,
    const std::shared_ptr<Gta5StreamState>& state) {
    registry.RegisterStep(
        std::make_shared<WorkflowGta5PlayerSwimStep>(logger, state));
    registry.RegisterStep(
        std::make_shared<WorkflowGta5PlayerFlyStep>(logger, state));
    registry.RegisterStep(
        std::make_shared<WorkflowGta5PlayerClimbStep>(logger, state));
}

}  // namespace sdl3cpp::services::impl
