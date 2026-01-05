#include "audio_command_service.hpp"
#include <filesystem>
#include <system_error>
#include <utility>

namespace sdl3cpp::services::impl {

AudioCommandService::AudioCommandService(std::shared_ptr<IConfigService> configService,
                                         std::shared_ptr<IAudioService> audioService,
                                         std::shared_ptr<ILogger> logger)
    : configService_(std::move(configService)),
      audioService_(std::move(audioService)),
      logger_(std::move(logger)) {
    if (logger_) {
        logger_->Trace("AudioCommandService", "AudioCommandService",
                       "configService=" + std::string(configService_ ? "set" : "null") +
                       ", audioService=" + std::string(audioService_ ? "set" : "null"));
    }
}

bool AudioCommandService::QueueAudioCommand(AudioCommandType type,
                                            const std::string& path,
                                            bool loop,
                                            std::string& error) {
    if (logger_) {
        logger_->Trace("AudioCommandService", "QueueAudioCommand",
                       "type=" + std::to_string(static_cast<int>(type)) +
                       ", path=" + path +
                       ", loop=" + std::string(loop ? "true" : "false"));
    }
    if (!audioService_) {
        error = "Audio service not available";
        return false;
    }
    if (!configService_) {
        error = "Config service not available";
        return false;
    }

    std::filesystem::path resolved(path);
    if (!resolved.is_absolute()) {
        std::filesystem::path scriptDir = configService_->GetScriptPath().parent_path();
        resolved = scriptDir / resolved;
    }

    std::error_code ec;
    resolved = std::filesystem::weakly_canonical(resolved, ec);
    if (ec) {
        error = "Failed to resolve audio path: " + ec.message();
        return false;
    }
    if (!std::filesystem::exists(resolved)) {
        error = "Audio file not found: " + resolved.string();
        return false;
    }

    try {
        if (type == AudioCommandType::Background) {
            audioService_->PlayBackground(resolved, loop);
        } else {
            audioService_->PlayEffect(resolved, loop);
        }
    } catch (const std::exception& ex) {
        error = ex.what();
        return false;
    }

    if (logger_) {
        logger_->Debug("Queued audio command: " + resolved.string());
    }

    return true;
}

bool AudioCommandService::StopBackground(std::string& error) {
    if (logger_) {
        logger_->Trace("AudioCommandService", "StopBackground");
    }
    if (!audioService_) {
        error = "Audio service not available";
        return false;
    }
    try {
        audioService_->StopBackground();
    } catch (const std::exception& ex) {
        error = ex.what();
        return false;
    }
    return true;
}

}  // namespace sdl3cpp::services::impl
