#include "services/interfaces/workflow/graphics/video_audio_tap.hpp"

#include <gtest/gtest.h>

#include <SDL3/SDL.h>

#include <vector>

namespace {

using sdl3cpp::services::impl::VideoAudioTap;

/// A logical device on SDL's dummy driver, playing @p level for ever.
struct Voice {
    SDL_AudioDeviceID device = 0;
    SDL_AudioStream* stream  = nullptr;
    explicit Voice(float level) {
        device =
            SDL_OpenAudioDevice(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, nullptr);
        const SDL_AudioSpec spec{SDL_AUDIO_F32, 2, 48000};
        stream = SDL_CreateAudioStream(&spec, nullptr);
        SDL_BindAudioStream(device, stream);
        const std::vector<float> samples(48000 * 2 * 4, level);
        SDL_PutAudioStreamData(stream, samples.data(),
                               int(samples.size() * sizeof(float)));
    }
    ~Voice() {
        SDL_DestroyAudioStream(stream);
        SDL_CloseAudioDevice(device);
    }
};

// Two devices mixed in the same passes come out as their sum; before
// the tap is on, nothing is kept.
TEST(VideoAudioTapTest, SumsEveryWatchedDevice) {
    SDL_SetHint(SDL_HINT_AUDIO_DRIVER, "dummy");
    ASSERT_TRUE(SDL_Init(SDL_INIT_AUDIO)) << SDL_GetError();
    {
        Voice quiet(0.1f), loud(0.25f);
        auto tap = std::make_unique<VideoAudioTap>();
        tap->Watch(quiet.device);
        tap->Watch(loud.device);
        tap->Watch(loud.device);  // a repeat changes nothing
        std::vector<float> got;
        int rate = 0, channels = 0;
        SDL_Delay(100);
        EXPECT_FALSE(tap->Take(got, rate, channels));

        tap->SetActive(true);
        SDL_Delay(300);
        tap->SetActive(false);
        ASSERT_TRUE(tap->Take(got, rate, channels));
        EXPECT_EQ(channels, 2);
        EXPECT_GT(rate, 0);
        ASSERT_FALSE(got.empty());
        EXPECT_NEAR(got[got.size() / 2], 0.35f, 1e-4f);
        tap.reset();  // unwatches before the devices close
    }
    SDL_Quit();
}

}  // namespace
