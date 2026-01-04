#pragma once

#include "../interfaces/i_audio_command_service.hpp"
#include "../interfaces/i_script_engine_service.hpp"
#include <memory>

namespace sdl3cpp::services::impl {

/**
 * @brief Script-facing audio command service implementation.
 */
class AudioCommandService : public IAudioCommandService {
public:
    explicit AudioCommandService(std::shared_ptr<IScriptEngineService> engineService);

    bool QueueAudioCommand(script::AudioManager::AudioCommandType type,
                           const std::string& path,
                           bool loop,
                           std::string& error) override;

private:
    std::shared_ptr<IScriptEngineService> engineService_;
};

}  // namespace sdl3cpp::services::impl
