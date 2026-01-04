#pragma once

#include "../../script/audio_manager.hpp"
#include <string>

namespace sdl3cpp::services {

/**
 * @brief Script-facing audio command service interface.
 */
class IAudioCommandService {
public:
    virtual ~IAudioCommandService() = default;

    virtual bool QueueAudioCommand(script::AudioManager::AudioCommandType type,
                                   const std::string& path,
                                   bool loop,
                                   std::string& error) = 0;
};

}  // namespace sdl3cpp::services
