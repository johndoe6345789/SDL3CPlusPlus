#include "sdl_audio_service.hpp"
#include <stdexcept>

namespace sdl3cpp::services::impl {

SdlAudioService::SdlAudioService(std::shared_ptr<ILogger> logger)
    : logger_(logger) {}

SdlAudioService::~SdlAudioService() {
    if (initialized_) {
        Shutdown();
    }
}

void SdlAudioService::Initialize() {
    logger_->TraceFunction(__func__);

    if (initialized_) {
        return;
    }

    // TODO: Initialize SDL audio subsystem
    logger_->Info("Audio service initialized (stub implementation)");
    initialized_ = true;
}

void SdlAudioService::Shutdown() noexcept {
    logger_->TraceFunction(__func__);

    if (!initialized_) {
        return;
    }

    // TODO: Shutdown SDL audio subsystem
    initialized_ = false;
    logger_->Info("Audio service shutdown (stub implementation)");
}

void SdlAudioService::PlayBackground(const std::filesystem::path& path, bool loop) {
    logger_->TraceFunction(__func__);

    if (!initialized_) {
        throw std::runtime_error("Audio service not initialized");
    }

    // TODO: Implement background music playback using SDL_mixer or similar
    logger_->Info("Playing background audio: " + path.string() + " (loop: " + std::to_string(loop) + ") - STUB");
}

void SdlAudioService::PlayEffect(const std::filesystem::path& path, bool loop) {
    logger_->TraceFunction(__func__);

    if (!initialized_) {
        throw std::runtime_error("Audio service not initialized");
    }

    // TODO: Implement sound effect playback
    logger_->Info("Playing effect audio: " + path.string() + " (loop: " + std::to_string(loop) + ") - STUB");
}

void SdlAudioService::StopBackground() {
    logger_->TraceFunction(__func__);

    if (!initialized_) {
        return;
    }

    // TODO: Stop background music
    logger_->Info("Stopping background audio - STUB");
}

void SdlAudioService::StopAll() {
    logger_->TraceFunction(__func__);

    if (!initialized_) {
        return;
    }

    // TODO: Stop all audio
    logger_->Info("Stopping all audio - STUB");
}

void SdlAudioService::SetVolume(float volume) {
    volume_ = std::clamp(volume, 0.0f, 1.0f);
    // Note: AudioPlayer doesn't expose volume control,
    // this would need to be added to AudioPlayer implementation
}

float SdlAudioService::GetVolume() const {
    return volume_;
}

bool SdlAudioService::IsBackgroundPlaying() const {
    // TODO: Check if background music is currently playing
    return false; // Stub implementation
}

}  // namespace sdl3cpp::services::impl
