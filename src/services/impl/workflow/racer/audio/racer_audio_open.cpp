#include "services/interfaces/workflow/racer/audio/racer_audio_mixer.hpp"

#include <SDL3/SDL_init.h>

namespace sdl3cpp::services::impl {
namespace {

/// Music for each planet, from data/wavs/Music; PodLoop1 elsewhere, and
/// Anakin's theme on the title screens ("Menu").
const char* MusicFor(const std::string& planet) {
    if (planet == "Menu") return "AnakinLoop.wav";
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
                           const std::string& planet, bool menu) {
    if (!SDL_InitSubSystem(SDL_INIT_AUDIO)) return false;
    open_ = true;
    menu_ = menu;
    const auto wavs = racerDir / "data" / "wavs";
    wavs_ = wavs;
    const auto sfx = wavs / "11K";
    const std::filesystem::path race[kVoices] = {
        wavs / "Music" / MusicFor(planet),
        sfx / "sfx_pod_jet_steady_loop.wav",
        sfx / "sfx_start_beep.wav",
        sfx / "sfx_start_game.wav",
        sfx / "sfx_crash_metal_scrape.wav",
        sfx / "sfx_explo_muffled_01.wav",
        wavs / "22K" / "sfx_weapon_tusken_gun.wav",
        wavs / "22K" / "sfx_geyser_vent.wav",
        wavs / "22K" / "sfx_crash_rock.wav", {}};
    // The title screens: music, no engine, then cursor, select and coin.
    const std::filesystem::path titles[kVoices] = {
        wavs / "Music" / MusicFor(planet), {},
        sfx / "sfx_select_softswitch1.wav",
        sfx / "sfx_select_pulse1.wav",
        sfx / "sfx_coin_roll_short.wav", {}, {}, {}, {}, {}};
    const auto& files = menu ? titles : race;
    const float gains[kVoices] = {0.45f, 0.6f, 0.8f, 0.9f, 0.7f,
                                  0.9f, 0.7f, 0.8f, 0.9f, 1.f};
    for (int v = 0; v < kVoices; ++v) {
        if (files[v].empty()) continue;
        clips_[v] = LoadRacerAudioClip(files[v]);
        streams_[v] = OpenRacerAudioStream(clips_[v]);
        if (streams_[v]) SDL_SetAudioStreamGain(streams_[v], gains[v]);
    }
    return streams_[kEngine] != nullptr || streams_[kMusic] != nullptr;
}

}  // namespace sdl3cpp::services::impl
