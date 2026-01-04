#include "script/audio_manager.hpp"
#include "app/audio_player.hpp"

#include <iostream>
#include <stdexcept>
#include <system_error>
#include <utility>

namespace sdl3cpp::script {

AudioManager::AudioManager(const std::filesystem::path& scriptDirectory)
    : scriptDirectory_(scriptDirectory) {}

void AudioManager::SetAudioPlayer(app::AudioPlayer* audioPlayer) {
    audioPlayer_ = audioPlayer;
    if (!audioPlayer_) {
        return;
    }
    for (const auto& command : pendingAudioCommands_) {
        try {
            ExecuteAudioCommand(audioPlayer_, command);
        } catch (const std::exception& exc) {
            std::cerr << "AudioPlayer: " << exc.what() << '\n';
        }
    }
    pendingAudioCommands_.clear();
}

bool AudioManager::QueueAudioCommand(AudioCommandType type, std::string path, bool loop, std::string& error) {
    if (audioPlayer_) {
        try {
            AudioCommand command{type, std::move(path), loop};
            ExecuteAudioCommand(audioPlayer_, command);
            return true;
        } catch (const std::exception& exc) {
            error = exc.what();
            return false;
        }
    }
    pendingAudioCommands_.push_back(AudioCommand{type, std::move(path), loop});
    return true;
}

void AudioManager::ExecuteAudioCommand(app::AudioPlayer* player, const AudioCommand& command) {
    auto resolved = ResolveScriptPath(command.path);
    if (!std::filesystem::exists(resolved)) {
        throw std::runtime_error("Audio file not found: " + resolved.string());
    }
    switch (command.type) {
        case AudioCommandType::Background:
            player->PlayBackground(resolved, command.loop);
            break;
        case AudioCommandType::Effect:
            player->PlayEffect(resolved, command.loop);
            break;
    }
}

std::filesystem::path AudioManager::ResolveScriptPath(const std::string& requested) const {
    std::filesystem::path resolved(requested);
    if (!resolved.is_absolute()) {
        resolved = scriptDirectory_ / resolved;
    }
    std::error_code ec;
    auto canonical = std::filesystem::weakly_canonical(resolved, ec);
    if (!ec) {
        resolved = canonical;
    }
    return resolved;
}

} // namespace sdl3cpp::script