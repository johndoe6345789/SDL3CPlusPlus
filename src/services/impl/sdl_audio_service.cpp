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

    audioPlayer_ = std::make_unique<app::AudioPlayer>();
    initialized_ = true;

    logger_->Info("Audio service initialized");
}

void SdlAudioService::Shutdown() noexcept {
    logger_->TraceFunction(__func__);

    if (!initialized_) {
        return;
    }

    audioPlayer_.reset();
    initialized_ = false;

    logger_->Info("Audio service shutdown");
}

void SdlAudioService::PlayBackground(const std::filesystem::path& path, bool loop) {
    logger_->TraceFunction(__func__);

    if (!audioPlayer_) {
        throw std::runtime_error("Audio service not initialized");
    }

    audioPlayer_->PlayBackground(path, loop);
}

void SdlAudioService::PlayEffect(const std::filesystem::path& path, bool loop) {
    logger_->TraceFunction(__func__);

    if (!audioPlayer_) {
        throw std::runtime_error("Audio service not initialized");
    }

    audioPlayer_->PlayEffect(path, loop);
}

void SdlAudioService::StopBackground() {
    logger_->TraceFunction(__func__);

    if (!audioPlayer_) {
        return;
    }

    // AudioPlayer doesn't have StopBackground(), so recreate to stop
    auto oldPlayer = std::move(audioPlayer_);
    oldPlayer.reset();
    audioPlayer_ = std::make_unique<app::AudioPlayer>();
}

void SdlAudioService::StopAll() {
    logger_->TraceFunction(__func__);

    if (!audioPlayer_) {
        return;
    }

    // Recreate player to stop all audio
    audioPlayer_.reset();
    audioPlayer_ = std::make_unique<app::AudioPlayer>();
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
    // AudioPlayer doesn't expose this state,
    // would need to be added to AudioPlayer implementation
    return false;
}

}  // namespace sdl3cpp::services::impl
