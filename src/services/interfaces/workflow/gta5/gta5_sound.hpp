#pragma once

#include "services/interfaces/i_logger.hpp"
#include "services/interfaces/workflow/quake3/q3_sound_bank.hpp"

#include <SDL3/SDL_audio.h>

#include <filesystem>
#include <memory>
#include <random>
#include <string>
#include <vector>

namespace sdl3cpp::services::impl {

using Gta5Clip = ::sdl3cpp::q3::Sound;

/// What the GTA sound step plays, a set per kind: a footfall or a stroke
/// picks one of its set at random, so no two sound quite the same.
struct Gta5Sounds {
    std::vector<Gta5Clip> steps;    // a foot on the ground
    std::vector<Gta5Clip> strokes;  // an arm through the water
    std::vector<Gta5Clip> splash;   // falling in
    std::vector<Gta5Clip> engine;   // a loop, pitched by the revs
    std::vector<Gta5Clip> water;    // a loop: the lapping of the shore
};

/// Every .wav in dir/steps, dir/strokes, dir/splash, dir/engine and
/// dir/water -- GTA's own, exported from its AWC banks -- or, where a
/// folder is empty or missing, synthesised stand-ins.
Gta5Sounds LoadGta5Sounds(const std::filesystem::path& dir,
                          const std::shared_ptr<ILogger>& logger);

/// A stand-in: mono float at 44.1 kHz. kind is steps, strokes, splash,
/// engine or water; variant seeds its noise. SynthGta5Wave is its raw
/// samples, SynthGta5Sound those levelled into a clip.
std::vector<float> SynthGta5Wave(const std::string& kind, int variant);
Gta5Clip SynthGta5Sound(const std::string& kind, int variant);

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
