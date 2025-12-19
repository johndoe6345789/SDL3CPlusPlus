#ifndef SDL3CPP_APP_AUDIO_PLAYER_HPP
#define SDL3CPP_APP_AUDIO_PLAYER_HPP

#include <cstdint>
#include <filesystem>
#include <memory>
#include <vector>

#include <SDL3/SDL.h>

namespace sdl3cpp::app {

class AudioPlayer {
public:
    explicit AudioPlayer(const std::filesystem::path& oggPath);
    ~AudioPlayer();

    AudioPlayer(const AudioPlayer&) = delete;
    AudioPlayer& operator=(const AudioPlayer&) = delete;

private:
    static void AudioStreamCallback(void* userdata, SDL_AudioStream* stream, int additionalAmount, int totalAmount);
    void FeedStream(SDL_AudioStream* stream, int totalAmount);

    SDL_AudioStream* stream_ = nullptr;
    std::vector<int16_t> buffer_;
    size_t positionBytes_ = 0;
    size_t bufferSizeBytes_ = 0;
};

} // namespace sdl3cpp::app

#endif // SDL3CPP_APP_AUDIO_PLAYER_HPP
