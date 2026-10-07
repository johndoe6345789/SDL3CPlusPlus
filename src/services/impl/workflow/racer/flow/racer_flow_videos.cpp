#include "services/interfaces/workflow/racer/flow/racer_flow_rules.hpp"

#include <SDL3/SDL_stdinc.h>

#include <string>

namespace sdl3cpp::services::impl {
namespace {

/// Each planet's flyover in data/anims, by the order of the planets'
/// bits in RacerProfile::planetsSeen. PlanetG repeats Baroonda's.
struct PlanetVideo {
    const char* planet;
    const char* video;
};
constexpr PlanetVideo kPlanets[] = {
    {"Tatooine", "PlanetTAT"}, {"Ando Prime", "PlanetA"},
    {"Aquilaris", "PlanetB"},  {"Ord Ibanna", "PlanetC"},
    {"Baroonda", "PlanetD"},   {"Mon Gazza", "PlanetE"},
    {"Oovo IV", "PlanetF"},    {"Malastare", "PlanetJ"}};

bool EnvOn(const char* name) {
    const char* value = SDL_getenv(name);
    return value && value[0] != 0 && std::string(value) != "0";
}

}  // namespace

bool RacerVideosWanted() {
    // Headless dev runs and the autopilot skip cutscenes unless
    // RACER_VIDEOS asks for them; RACER_SKIP_VIDEOS skips them always.
    if (EnvOn("RACER_SKIP_VIDEOS")) return false;
    if (EnvOn("RACER_VIDEOS")) return true;
    return !EnvOn("SDL3CPP_HEADLESS") && !EnvOn("RACER_AUTOPILOT");
}

void PlayRacerVideosThen(RacerFlow& flow, std::vector<std::string> videos,
                         RacerPhase after) {
    if (videos.empty() || !RacerVideosWanted()) {
        flow.phase = after;
        return;
    }
    flow.videos = std::move(videos);
    flow.afterVideos = after;
    flow.skipVideo = false;
    flow.phase = RacerPhase::Cutscene;
}

void QueueRacerPlanetIntro(RacerFlow& flow, const std::string& planet) {
    for (std::size_t i = 0; i < std::size(kPlanets); ++i) {
        const std::uint32_t bit = 1u << i;
        if (planet != kPlanets[i].planet) continue;
        if (flow.profile.planetsSeen & bit) return;
        flow.profile.planetsSeen |= bit;
        SaveRacerProfile(flow.profile, RacerProfilePath());
        PlayRacerVideosThen(flow, {kPlanets[i].video}, flow.phase);
        return;
    }
}

}  // namespace sdl3cpp::services::impl
