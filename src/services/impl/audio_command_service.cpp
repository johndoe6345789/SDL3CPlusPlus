#include "audio_command_service.hpp"
#include <utility>

namespace sdl3cpp::services::impl {

AudioCommandService::AudioCommandService(std::shared_ptr<IScriptEngineService> engineService)
    : engineService_(std::move(engineService)) {
}

bool AudioCommandService::QueueAudioCommand(script::AudioManager::AudioCommandType type,
                                            const std::string& path,
                                            bool loop,
                                            std::string& error) {
    return engineService_->GetEngine().QueueAudioCommand(type, path, loop, error);
}

}  // namespace sdl3cpp::services::impl
