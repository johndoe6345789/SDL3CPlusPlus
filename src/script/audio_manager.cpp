#include "script/audio_manager.hpp"
#include "logging/logger.hpp"

#include <iostream>
#include <stdexcept>
#include <system_error>
#include <utility>

namespace sdl3cpp::script {

AudioManager::AudioManager(const std::filesystem::path& scriptDirectory)
    : scriptDirectory_(scriptDirectory) {
    sdl3cpp::logging::TraceGuard trace;
}

void AudioManager::SetAudioPlayer(app::AudioPlayer* audioPlayer) {
    // Stub - audio functionality now handled through services
    sdl3cpp::logging::Logger::GetInstance().Trace("AudioManager::SetAudioPlayer: Stub implementation - using services now");
}

bool AudioManager::QueueAudioCommand(AudioCommandType type, std::string path, bool loop, std::string& error) {
    // Stub implementation - audio functionality now handled through services
    sdl3cpp::logging::Logger::GetInstance().Trace("AudioManager::QueueAudioCommand: Stub - " + path + " (type: " + 
        (type == AudioCommandType::Background ? "background" : "effect") + ", loop: " + (loop ? "true" : "false") + ")");
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