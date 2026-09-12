#include "services/interfaces/workflow/gta5/audio/gta5_sound.hpp"

#include "services/interfaces/workflow/quake3/q3_sound_playback.hpp"

#include <algorithm>
#include <cmath>
#include <cstring>

namespace sdl3cpp::services::impl {

Gta5Clip SynthGta5Sound(const std::string& kind, int variant) {
    std::vector<float> wave = SynthGta5Wave(kind, variant);
    float mean = 0.f, peak = 1e-6f;  // centred, and peaking at 0.8
    for (float s : wave) mean += s / wave.size();
    for (float& s : wave) peak = std::max(peak, std::abs(s -= mean));
    for (float& s : wave) s *= 0.8f / peak;
    Gta5Clip clip;
    clip.spec = {SDL_AUDIO_F32, 1, 44100};
    clip.pcm.resize(wave.size() * sizeof(float));
    std::memcpy(clip.pcm.data(), wave.data(), clip.pcm.size());
    return clip;
}

void FeedGta5Loop(Gta5Loop& loop, const Gta5Clip& clip,
                  SDL_AudioDeviceID device, const SDL_AudioSpec& spec,
                  float gain, float ratio) {
    if (clip.pcm.empty()) return;
    if (!loop.stream) {
        loop.stream = SDL_CreateAudioStream(&clip.spec, &spec);
        if (!loop.stream) return;
        if (!SDL_BindAudioStream(device, loop.stream)) {
            SDL_DestroyAudioStream(loop.stream);
            loop.stream = nullptr;
            return;
        }
    }
    // Pitch and gain act as the device pulls, so they follow the revs
    // at once however much is queued.
    SDL_SetAudioStreamGain(loop.stream, std::max(0.f, gain));
    SDL_SetAudioStreamFrequencyRatio(loop.stream, std::clamp(ratio, .1f, 8.f));
    const int bytes = static_cast<int>(clip.pcm.size());
    for (int i = 0; i < 2 && SDL_GetAudioStreamQueued(loop.stream) < bytes;
         ++i) {
        SDL_PutAudioStreamData(loop.stream, clip.pcm.data(), bytes);
    }
}

void PlayGta5Clip(std::vector<SDL_AudioStream*>& playing,
                  const std::vector<Gta5Clip>& set, std::mt19937& rng,
                  SDL_AudioDeviceID device, const SDL_AudioSpec& spec,
                  float gain, float ratio) {
    if (set.empty() || gain <= 0.f) return;
    std::uniform_int_distribution<std::size_t> pick(0, set.size() - 1);
    SDL_AudioStream* stream =
        PlaySoundOnDevice(set[pick(rng)], device, spec, nullptr);
    if (!stream) return;
    SDL_SetAudioStreamGain(stream, gain);
    SDL_SetAudioStreamFrequencyRatio(stream, ratio);
    playing.push_back(stream);
}

}  // namespace sdl3cpp::services::impl
