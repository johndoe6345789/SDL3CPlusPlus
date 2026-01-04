#pragma once

#include "../interfaces/i_audio_service.hpp"
#include "../interfaces/i_logger.hpp"
#include "../../di/lifecycle.hpp"
#include <memory>
#include <SDL3/SDL.h>
#include <vorbis/vorbisfile.h>
#include <filesystem>
#include <vector>
#include <atomic>
#include <mutex>

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

    // Update method to be called regularly (e.g., from main loop)
    void Update();

private:
    struct AudioData {
        OggVorbis_File vorbisFile;
        bool isOpen = false;
        bool loop = false;
        size_t position = 0;
    };

    bool LoadAudioFile(const std::filesystem::path& path, AudioData& audioData);
    void CleanupAudioData(AudioData& audioData);

    std::shared_ptr<ILogger> logger_;
    float volume_ = 1.0f;
    bool initialized_ = false;

    SDL_AudioStream* audioStream_ = nullptr;

    std::unique_ptr<AudioData> backgroundAudio_;
    std::mutex audioMutex_;
};

}  // namespace sdl3cpp::services::impl
