#include "services/interfaces/workflow/gta5/player/gta5_player_climb_step.hpp"

#include <utility>

namespace sdl3cpp::services::impl {

WorkflowGta5PlayerClimbStep::WorkflowGta5PlayerClimbStep(
    std::shared_ptr<ILogger> logger, std::shared_ptr<Gta5StreamState> state)
    : logger_(std::move(logger)), state_(std::move(state)) {}

std::string WorkflowGta5PlayerClimbStep::GetPluginId() const {
    return "gta5.player.climb";
}

}  // namespace sdl3cpp::services::impl
