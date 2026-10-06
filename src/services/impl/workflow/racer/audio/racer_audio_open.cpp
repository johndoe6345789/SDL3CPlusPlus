#include "services/interfaces/workflow/racer/audio/racer_audio_mixer.hpp"

#include <SDL3/SDL_init.h>

namespace sdl3cpp::services::impl {
namespace {

/// Music for each planet, from data/wavs/Music; PodLoop1 elsewhere.
const char* MusicFor(const std::string& planet) {
    if (planet == "Tatooine") return "mt01desert.wav";
    if (planet == "Aquilaris") return "mb00aquilarisintro.wav";
    if (planet == "Mon Gazza") return "me00spiceintro.wav";
    if (planet == "Baroonda") return "mx091lavacaves.wav";
    return "PodLoop1.wav";
}

}  // namespace

RacerAudioMixer::~RacerAudioMixer() {
    for (SDL_AudioStream* stream : streams_) {
        if (stream) SDL_DestroyAudioStream(stream);
    }
    if (open_) SDL_QuitSubSystem(SDL_INIT_AUDIO);
}

bool RacerAudioMixer::Open(const std::filesystem::path& racerDir,
                           const std::string& planet) {
    if (!SDL_InitSubSystem(SDL_INIT_AUDIO)) return false;
    open_ = true;
    const auto wavs = racerDir / "data" / "wavs";
    const std::filesystem::path files[kVoices] = {
        wavs / "Music" / MusicFor(planet),
        wavs / "11K" / "sfx_pod_jet_steady_loop.wav",
        wavs / "11K" / "sfx_start_beep.wav",
        wavs / "11K" / "sfx_start_game.wav",
        wavs / "11K" / "sfx_crash_metal_scrape.wav",
        wavs / "11K" / "sfx_explo_muffled_01.wav"};
    const float gains[kVoices] = {0.45f, 0.6f, 0.8f, 0.9f, 0.7f, 0.9f};
    for (int v = 0; v < kVoices; ++v) {
        clips_[v] = LoadRacerAudioClip(files[v]);
        streams_[v] = OpenRacerAudioStream(clips_[v]);
        if (streams_[v]) SDL_SetAudioStreamGain(streams_[v], gains[v]);
    }
    return streams_[kEngine] != nullptr || streams_[kMusic] != nullptr;
}

}  // namespace sdl3cpp::services::impl
