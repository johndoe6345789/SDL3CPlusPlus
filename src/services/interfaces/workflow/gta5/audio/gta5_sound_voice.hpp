#pragma once

#include "services/interfaces/workflow/quake3/audio/q3_sound_bank.hpp"

#include <SDL3/SDL_audio.h>

#include <random>
#include <vector>

namespace sdl3cpp::services::impl {

using Gta5Clip = ::sdl3cpp::q3::Sound;

/// A looping voice, kept a clip ahead at this frame's gain and pitch
/// (a frequency ratio); opened on first use.
struct Gta5Loop {
    SDL_AudioStream* stream{nullptr};
};
void FeedGta5Loop(Gta5Loop& loop, const Gta5Clip& clip,
                  SDL_AudioDeviceID device, const SDL_AudioSpec& spec,
                  float gain, float ratio);

/// One of the set, once, at a gain and pitch. Finished voices are left
/// in `playing` for ReapFinishedSoundStreams.
void PlayGta5Clip(std::vector<SDL_AudioStream*>& playing,
                  const std::vector<Gta5Clip>& set, std::mt19937& rng,
                  SDL_AudioDeviceID device, const SDL_AudioSpec& spec,
                  float gain, float ratio);

}  // namespace sdl3cpp::services::impl
