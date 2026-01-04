#pragma once

#include <lua.hpp>

#include <filesystem>
#include <string>
#include <vector>

namespace sdl3cpp::app {
class AudioPlayer;
}

namespace sdl3cpp::script {

class AudioManager {
public:
    enum class AudioCommandType {
        Background,
        Effect
    };

    struct AudioCommand {
        AudioCommandType type;
        std::string path;
        bool loop;
    };

    explicit AudioManager(const std::filesystem::path& scriptDirectory);

    void SetAudioPlayer(app::AudioPlayer* audioPlayer);
    bool QueueAudioCommand(AudioCommandType type, std::string path, bool loop, std::string& error);

private:
    std::filesystem::path scriptDirectory_;
    app::AudioPlayer* audioPlayer_ = nullptr;
    std::vector<AudioCommand> pendingAudioCommands_;

    void ExecuteAudioCommand(app::AudioPlayer* player, const AudioCommand& command);
    std::filesystem::path ResolveScriptPath(const std::string& requested) const;
};

} // namespace sdl3cpp::script