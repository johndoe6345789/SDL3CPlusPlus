#pragma once

#include "services/interfaces/i_logger.hpp"
#include "services/interfaces/workflow/gta5/audio/gta5_sound_voice.hpp"

#include <filesystem>
#include <memory>
#include <string>
#include <vector>

namespace sdl3cpp::services::impl {

/// What the GTA sound step plays, a set per kind: a footfall or a stroke
/// picks one of its set at random, so no two sound quite the same.
struct Gta5Sounds {
    std::vector<Gta5Clip> steps;     // a foot on dry ground
    std::vector<Gta5Clip> wetSteps;  // a foot in shallow water
    std::vector<Gta5Clip> strokes;   // an arm through the water
    std::vector<Gta5Clip> splash;    // falling in
    std::vector<Gta5Clip> engine;    // a loop, pitched by the revs
    std::vector<Gta5Clip> water;     // a loop: the lapping of the shore
    std::vector<Gta5Clip> shots;     // GTA's own gunfire
    std::vector<Gta5Clip> blasts;    // and its explosions
};

/// Every .wav in dir/steps, dir/wet_steps, dir/strokes, dir/splash,
/// dir/engine and dir/water -- GTA's own, exported from its AWC banks
/// -- or, where a folder is empty or missing, synthesised stand-ins.
/// Wet steps with none exported are the dry ones.
Gta5Sounds LoadGta5Sounds(const std::filesystem::path& dir,
                          const std::shared_ptr<ILogger>& logger);

/// The decodable .wav files in @p folder, in no particular order.
std::vector<Gta5Clip> LoadGta5SoundSet(const std::filesystem::path& folder);

/// A .wav, whatever the case of its extension.
bool IsGta5Wav(const std::filesystem::path& path);

/// A stand-in: mono float at 44.1 kHz. kind is steps, strokes, splash,
/// engine or water; variant seeds its noise. SynthGta5Wave is its raw
/// samples, SynthGta5Sound those levelled into a clip.
std::vector<float> SynthGta5Wave(const std::string& kind, int variant);
Gta5Clip SynthGta5Sound(const std::string& kind, int variant);

}  // namespace sdl3cpp::services::impl
