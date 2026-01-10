#pragma once

#include "services/interfaces/i_audio_service.hpp"
#include "services/interfaces/i_logger.hpp"
#include "../../../di/lifecycle.hpp"
#include <memory>
#include <SDL3/SDL.h>
#include <vorbis/vorbisfile.h>
#include <filesystem>
#include <cstdint>
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
    void Update() override;

private:
    struct AudioData {
        OggVorbis_File vorbisFile{};
        SDL_AudioStream* convertStream = nullptr;
        SDL_AudioSpec sourceSpec{};
        bool isOpen = false;
        bool loop = false;
        bool finished = false;
    };

    bool LoadAudioFile(const std::filesystem::path& path, AudioData& audioData);
    void CleanupAudioData(AudioData& audioData);
    int ReadStreamSamples(AudioData& audioData, std::vector<int16_t>& output, int frames);

    std::shared_ptr<ILogger> logger_;
    float volume_ = 1.0f;
    bool initialized_ = false;

    SDL_AudioStream* audioStream_ = nullptr;
    SDL_AudioSpec mixSpec_{};

    std::unique_ptr<AudioData> backgroundAudio_;
    std::vector<std::unique_ptr<AudioData>> effectAudio_;
    std::mutex audioMutex_;
    std::vector<int32_t> mixBuffer_;
    std::vector<int16_t> tempBuffer_;
    std::vector<int16_t> outputBuffer_;
};

}  // namespace sdl3cpp::services::impl
