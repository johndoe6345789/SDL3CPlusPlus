#pragma once

#include "../interfaces/i_audio_service.hpp"
#include "../interfaces/i_logger.hpp"
#include "../../app/audio_player.hpp"
#include "../../di/lifecycle.hpp"
#include <memory>

namespace sdl3cpp::services::impl {

/**
 * @brief SDL audio service implementation.
 *
 * Small wrapper service (~80 lines) around AudioPlayer.
 * Provides thread-safe audio playback for background music and sound effects.
 */
class SdlAudioService : public IAudioService,
                        public di::IInitializable,
                        public di::IShutdownable {
public:
    explicit SdlAudioService(std::shared_ptr<ILogger> logger);
    ~SdlAudioService() override;

    // IAudioService interface
    void Initialize() override;
    void Shutdown() noexcept override;

    void PlayBackground(const std::filesystem::path& path, bool loop = true) override;
    void PlayEffect(const std::filesystem::path& path, bool loop = false) override;
    void StopBackground() override;
    void StopAll() override;

    void SetVolume(float volume) override;
    float GetVolume() const override;
    bool IsBackgroundPlaying() const override;

private:
    std::shared_ptr<ILogger> logger_;
    std::unique_ptr<app::AudioPlayer> audioPlayer_;
    float volume_ = 1.0f;
    bool initialized_ = false;
};

}  // namespace sdl3cpp::services::impl
