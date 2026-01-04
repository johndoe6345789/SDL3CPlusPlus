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

private:
    struct AudioData {
        std::vector<char> buffer;
        size_t position = 0;
        bool loop = false;
        OggVorbis_File vorbisFile;
        bool isOpen = false;
    };

    static void AudioCallback(void* userdata, Uint8* stream, int len);
    bool LoadAudioFile(const std::filesystem::path& path, AudioData& audioData);
    void CleanupAudioData(AudioData& audioData);

    std::shared_ptr<ILogger> logger_;
    float volume_ = 1.0f;
    bool initialized_ = false;

    SDL_AudioSpec audioSpec_;
    SDL_AudioDeviceID audioDevice_ = 0;

    std::unique_ptr<AudioData> backgroundAudio_;
    std::mutex audioMutex_;
};

}  // namespace sdl3cpp::services::impl
