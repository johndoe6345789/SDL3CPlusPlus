#pragma once

#include <SDL3/SDL_audio.h>

#include <cstdint>
#include <filesystem>
#include <vector>

namespace sdl3cpp::services::impl {

/// One sound from the install, as raw 16-bit PCM with its format.
struct RacerAudioClip {
    SDL_AudioSpec spec{};
    std::vector<std::uint8_t> bytes;
    bool Loaded() const { return !bytes.empty(); }
};

/// Whether the game may make sound: not in headless dev runs (unless
/// SDL's dummy driver is chosen, which plays to nowhere) nor with
/// RACER_MUTE set.
bool RacerAudioAllowed();

/// Reads a 16-bit PCM WAV. Empty when missing or in another format.
RacerAudioClip LoadRacerAudioClip(const std::filesystem::path& path);

/// A playback stream on the default device for one clip format.
/// Null when audio is unavailable.
SDL_AudioStream* OpenRacerAudioStream(const RacerAudioClip& clip);

/// Keeps a looping stream fed: queues the clip again whenever less than
/// half a second remains.
void FeedRacerAudioLoop(SDL_AudioStream* stream, const RacerAudioClip& clip);

/// Plays the clip once on its stream, replacing whatever was queued.
void PlayRacerAudioOnce(SDL_AudioStream* stream, const RacerAudioClip& clip);

}  // namespace sdl3cpp::services::impl
