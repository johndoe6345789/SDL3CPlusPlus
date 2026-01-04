#pragma once

#include "../interfaces/i_audio_command_service.hpp"
#include "../interfaces/i_audio_service.hpp"
#include "../interfaces/i_config_service.hpp"
#include "../interfaces/i_logger.hpp"
#include <memory>

namespace sdl3cpp::services::impl {

/**
 * @brief Script-facing audio command service implementation.
 */
class AudioCommandService : public IAudioCommandService {
public:
    AudioCommandService(std::shared_ptr<IConfigService> configService,
                        std::shared_ptr<IAudioService> audioService,
                        std::shared_ptr<ILogger> logger);

    bool QueueAudioCommand(AudioCommandType type,
                           const std::string& path,
                           bool loop,
                           std::string& error) override;

private:
    std::shared_ptr<IConfigService> configService_;
    std::shared_ptr<IAudioService> audioService_;
    std::shared_ptr<ILogger> logger_;
};

}  // namespace sdl3cpp::services::impl
