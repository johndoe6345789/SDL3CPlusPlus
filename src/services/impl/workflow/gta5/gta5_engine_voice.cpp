#include "services/interfaces/workflow/gta5/gta5_engine_bank.hpp"

#include <algorithm>

namespace sdl3cpp::services::impl {
namespace {

constexpr std::size_t kFade = 48;  // a millisecond at 48 kHz

/// Grain g, faded in over the recording's continuation past the last
/// grain: a jump in the recording is a crossfade, not a click, and one
/// grain after the next is seamless, as recorded.
void Append(Gta5EngineVoice& voice, const Gta5EngineSweep& sweep,
            std::size_t g) {
    const std::size_t size = sweep.samples.size();
    const std::size_t begin = std::min<std::size_t>(sweep.starts[g], size);
    const std::size_t end = std::min<std::size_t>(sweep.starts[g + 1], size);
    const std::size_t at = voice.out.size();
    voice.out.insert(voice.out.end(), sweep.samples.begin() + begin,
                     sweep.samples.begin() + end);
    const std::size_t fade = std::min(voice.tail.size(), end - begin);
    for (std::size_t i = 0; i < fade; ++i) {
        const float in = (float(i) + 0.5f) / float(voice.tail.size());
        voice.out[at + i] = voice.out[at + i] * in + voice.tail[i] * (1 - in);
    }
    voice.tail.assign(kFade, 0.f);
    for (std::size_t i = 0; i < kFade && end + i < size; ++i) {
        voice.tail[i] = sweep.samples[end + i];
    }
    voice.grain = g;
    voice.sweep = &sweep;
}

}  // namespace

void FeedGta5Engine(Gta5EngineVoice& voice, const Gta5EngineBank& bank,
                    SDL_AudioDeviceID device, const SDL_AudioSpec& spec,
                    float revs, bool throttle, float gain) {
    if (!bank.loaded) return;
    if (!voice.stream) {
        const SDL_AudioSpec source{SDL_AUDIO_F32, 1, bank.accel.rate};
        voice.stream = SDL_CreateAudioStream(&source, &spec);
        if (!voice.stream) return;
        if (!SDL_BindAudioStream(device, voice.stream)) {
            SDL_DestroyAudioStream(voice.stream);
            voice.stream = nullptr;
            return;
        }
    }
    SDL_SetAudioStreamGain(voice.stream, gain);
    const Gta5EngineSweep& sweep = revs < 0.05f && !throttle ? bank.idle
                                   : throttle                ? bank.accel
                                                             : bank.decel;
    // About 60 ms ahead: the revs are heard that late at most.
    const int ahead = bank.accel.rate / 16 * int(sizeof(float));
    while (SDL_GetAudioStreamQueued(voice.stream) +
               int(voice.out.size() * sizeof(float)) < ahead) {
        Append(voice, sweep, NextGta5Grain(voice, sweep, revs));
    }
    SDL_PutAudioStreamData(voice.stream, voice.out.data(),
                           int(voice.out.size() * sizeof(float)));
    voice.out.clear();
}

}  // namespace sdl3cpp::services::impl
