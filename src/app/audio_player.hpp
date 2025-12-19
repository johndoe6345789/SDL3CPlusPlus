#ifndef SDL3CPP_APP_AUDIO_PLAYER_HPP
#define SDL3CPP_APP_AUDIO_PLAYER_HPP

#include <cstdint>
#include <filesystem>
#include <mutex>
#include <optional>
#include <vector>

#include <SDL3/SDL.h>

namespace sdl3cpp::app {

class AudioPlayer {
public:
    AudioPlayer();
    ~AudioPlayer();

    AudioPlayer(const AudioPlayer&) = delete;
    AudioPlayer& operator=(const AudioPlayer&) = delete;

    void PlayBackground(const std::filesystem::path& path, bool loop = true);
    void PlayEffect(const std::filesystem::path& path, bool loop = false);

private:
    struct AudioVoice {
        std::vector<int16_t> data;
        size_t position = 0;
        bool loop = false;
        bool active = true;
    };

    static void AudioStreamCallback(void* userdata, SDL_AudioStream* stream, int additionalAmount, int totalAmount);
    void FeedStream(SDL_AudioStream* stream, int totalAmount);
    void EnsureStream(int sampleRate, int channels);
    void AddVoiceSamples(AudioVoice& voice, std::vector<int32_t>& mixBuffer, size_t sampleCount);

    SDL_AudioStream* stream_ = nullptr;
    int sampleRate_ = 0;
    int channels_ = 0;

    std::optional<AudioVoice> backgroundVoice_;
    std::vector<AudioVoice> effectVoices_;

    std::mutex voicesMutex_;
    std::vector<int32_t> mixBuffer_;
    std::vector<int16_t> outputBuffer_;
};

} // namespace sdl3cpp::app

#endif // SDL3CPP_APP_AUDIO_PLAYER_HPP
