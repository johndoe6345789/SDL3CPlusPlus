#pragma once

#include <SDL3/SDL_audio.h>

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <random>
#include <vector>

namespace sdl3cpp::services::impl {

/// One of a GTA granular engine bank's recordings: a sweep through the
/// revs cut into pitch-synchronous grains -- grain i runs from starts[i]
/// to starts[i + 1] -- each marked with the revs it was recorded at,
/// rising (accel) or falling (decel).
struct Gta5EngineSweep {
    std::vector<float> samples;  // mono
    std::vector<std::uint32_t> starts;
    std::vector<float> revs;
    int rate{48000};
};

/// A car's engine as GTA records it (STREAMED_VEHICLES_GRANULAR): on and
/// off the throttle, and idling.
struct Gta5EngineBank {
    Gta5EngineSweep accel, decel, idle;
    bool loaded{false};
};

/// folder/grains.json -- {"engine_accel": {"file", "grains": [[start,
/// revs], ...]}, "engine_decel": ..., "idle": ...} -- and its WAVs.
bool LoadGta5EngineBank(const std::filesystem::path& folder,
                        Gta5EngineBank& bank);

/// The engine playing: grains chosen for the revs, queued a little
/// ahead, each faded into the last one's continuation.
struct Gta5EngineVoice {
    SDL_AudioStream* stream{nullptr};
    const Gta5EngineSweep* sweep{nullptr};
    std::size_t grain{0};
    std::vector<float> out;
    std::vector<float> tail;  // the recording just past the last grain
    std::mt19937 rng{1};
};

/// The grain to play next for `revs`: on through the recording towards
/// them -- the engine winding up or down as recorded -- then hovering
/// about them. The idle loop just goes round.
std::size_t NextGta5Grain(Gta5EngineVoice& voice,
                          const Gta5EngineSweep& sweep, float revs);

/// Tops the voice up for `revs` (0 idle .. 1 the redline): the accel
/// sweep on the throttle, the decel sweep off it, idle at rest.
void FeedGta5Engine(Gta5EngineVoice& voice, const Gta5EngineBank& bank,
                    SDL_AudioDeviceID device, const SDL_AudioSpec& spec,
                    float revs, bool throttle, float gain);

}  // namespace sdl3cpp::services::impl
