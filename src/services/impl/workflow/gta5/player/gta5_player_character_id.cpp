#include "services/interfaces/workflow/gta5/player/gta5_player_character_step.hpp"

#include <utility>

namespace sdl3cpp::services::impl {

WorkflowGta5PlayerCharacterStep::WorkflowGta5PlayerCharacterStep(
    std::shared_ptr<ILogger> logger, std::shared_ptr<Gta5StreamState> state)
    : logger_(std::move(logger)), state_(std::move(state)) {}

std::string WorkflowGta5PlayerCharacterStep::GetPluginId() const {
    return "gta5.player.character";
}

}  // namespace sdl3cpp::services::impl
