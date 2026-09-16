#include "services/interfaces/workflow/gta5/player/gta5_player_camera_step.hpp"

#include <utility>

namespace sdl3cpp::services::impl {

WorkflowGta5PlayerCameraStep::WorkflowGta5PlayerCameraStep(
    std::shared_ptr<ILogger> logger, std::shared_ptr<Gta5StreamState> state)
    : logger_(std::move(logger)), state_(std::move(state)) {}

std::string WorkflowGta5PlayerCameraStep::GetPluginId() const {
    return "gta5.player.camera";
}

}  // namespace sdl3cpp::services::impl
