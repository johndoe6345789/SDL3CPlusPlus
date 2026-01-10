#include "soundboard_state_service.hpp"

#include <utility>

namespace sdl3cpp::services::impl {

SoundboardStateService::SoundboardStateService(std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)),
      statusMessage_("Select a clip to play") {
    if (logger_) {
        logger_->Trace("SoundboardStateService", "SoundboardStateService",
                       "status=" + statusMessage_);
    }
}

void SoundboardStateService::SetStatusMessage(std::string message) {
    statusMessage_ = std::move(message);
    if (logger_) {
        logger_->Trace("SoundboardStateService", "SetStatusMessage",
                       "status=" + statusMessage_);
    }
}

const std::string& SoundboardStateService::GetStatusMessage() const {
    return statusMessage_;
}

}  // namespace sdl3cpp::services::impl
