#pragma once

#include <SDL3/SDL_audio.h>

#include <memory>
#include <mutex>
#include <vector>

namespace sdl3cpp::services::impl {

/**
 * @brief What the game plays, as it plays it: SDL's post-mix output of
 * each logical audio device it is told of, summed.
 *
 * SDL mixes every logical device on the audio thread, one after
 * another, a buffer at a time. The sum of a pass is complete when a
 * device comes round again, and only then moves to where Take finds
 * it. Thread-safe; Watch and the destructor belong to one thread.
 */
class VideoAudioTap {
public:
    ~VideoAudioTap();

    /// Starts listening to @p device. Repeats are ignored.
    void Watch(SDL_AudioDeviceID device);
    /// Nothing is kept while off; turning off finishes the pass.
    void SetActive(bool active);
    /// Moves out what has been kept: interleaved float @p rate Hz
    /// samples of @p channels. False when there is nothing.
    bool Take(std::vector<float>& samples, int& rate, int& channels);

private:
    struct Source {
        VideoAudioTap* tap;
        SDL_AudioDeviceID device;
    };
    static void SDLCALL OnMix(void* user, const SDL_AudioSpec* spec,
                              float* buffer, int bytes);
    void Mix(SDL_AudioDeviceID device, const SDL_AudioSpec& spec,
             const float* samples, int count);
    void FinishPass();  // with mutex_ held

    std::vector<std::unique_ptr<Source>> sources_;
    std::mutex mutex_;
    std::vector<float> pass_;  // the pass being summed
    std::vector<SDL_AudioDeviceID> inPass_;
    std::vector<float> kept_;  // whole passes, for Take
    int rate_     = 0;
    int channels_ = 0;
    bool active_  = false;
};

}  // namespace sdl3cpp::services::impl
