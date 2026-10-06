#include "services/interfaces/workflow/racer/render/racer_hud_step.hpp"

#include <cmath>

namespace sdl3cpp::services::impl {
namespace {

constexpr float kGoSeconds = 1.2f;
constexpr float kFinalLapSeconds = 2.5f;

}  // namespace

std::string FormatRacerBanner(const RacerRaceState& race) {
    if (race.countdown > 0.f) {
        return std::to_string(static_cast<int>(std::ceil(race.countdown)));
    }
    if (race.finished) return "FINISHED";
    if (race.raceTime < kGoSeconds) return "GO!";
    if (race.lapsTotal > 1 && race.lap == race.lapsTotal &&
        race.lapTime < kFinalLapSeconds) {
        return "FINAL LAP";
    }
    return "";
}

}  // namespace sdl3cpp::services::impl
