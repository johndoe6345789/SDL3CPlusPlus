#include "sdl_audio_service.hpp"
#include "../../logging/logger.hpp"
#include <stdexcept>

namespace sdl3cpp::services::impl {

SdlAudioService::SdlAudioService() = default;

SdlAudioService::~SdlAudioService() {
    if (initialized_) {
        Shutdown();
    }
}

void SdlAudioService::Initialize() {
    logging::TraceGuard trace;

    if (initialized_) {
        return;
    }

    audioPlayer_ = std::make_unique<app::AudioPlayer>();
    initialized_ = true;

    logging::Logger::GetInstance().Info("Audio service initialized");
}

void SdlAudioService::Shutdown() noexcept {
    logging::TraceGuard trace;

    if (!initialized_) {
        return;
    }

    audioPlayer_.reset();
    initialized_ = false;

    logging::Logger::GetInstance().Info("Audio service shutdown");
}

void SdlAudioService::PlayBackground(const std::filesystem::path& path, bool loop) {
    logging::TraceGuard trace;

    if (!audioPlayer_) {
        throw std::runtime_error("Audio service not initialized");
    }

    audioPlayer_->PlayBackground(path, loop);
}

void SdlAudioService::PlayEffect(const std::filesystem::path& path, bool loop) {
    logging::TraceGuard trace;

    if (!audioPlayer_) {
        throw std::runtime_error("Audio service not initialized");
    }

    audioPlayer_->PlayEffect(path, loop);
}

void SdlAudioService::StopBackground() {
    logging::TraceGuard trace;

    if (!audioPlayer_) {
        return;
    }

    // AudioPlayer doesn't have StopBackground(), so recreate to stop
    auto oldPlayer = std::move(audioPlayer_);
    oldPlayer.reset();
    audioPlayer_ = std::make_unique<app::AudioPlayer>();
}

void SdlAudioService::StopAll() {
    logging::TraceGuard trace;

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
