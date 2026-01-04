#pragma once

#include <string>

namespace sdl3cpp::services {

/**
 * @brief Audio command type for script bindings.
 */
enum class AudioCommandType {
    Background,
    Effect
};

/**
 * @brief Script-facing audio command service interface.
 */
class IAudioCommandService {
public:
    virtual ~IAudioCommandService() = default;

    virtual bool QueueAudioCommand(AudioCommandType type,
                                   const std::string& path,
                                   bool loop,
                                   std::string& error) = 0;
};

}  // namespace sdl3cpp::services
