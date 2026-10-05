#include "services/interfaces/workflow/racer/render/racer_hud_step.hpp"

#include <cmath>
#include <cstdio>

namespace sdl3cpp::services::impl {
namespace {

std::string Clock(float seconds) {
    if (seconds <= 0.f) return "-:--.--";
    const int minutes = static_cast<int>(seconds / 60.f);
    char text[16];
    std::snprintf(text, sizeof(text), "%d:%05.2f", minutes,
                  seconds - 60.f * minutes);
    return text;
}

std::string HeatBar(float heat) {
    const int filled = static_cast<int>(std::lround(heat * 10.f));
    return std::string(filled, '#') + std::string(10 - filled, '-');
}

}  // namespace

std::string FormatRacerHud(const RacerRaceState& race,
                           const RacerPodState& pod) {
    if (race.countdown > 0.f) {
        return "    " + std::to_string(
                            static_cast<int>(std::ceil(race.countdown)));
    }
    if (race.finished) {
        return "FINISHED " + Clock(race.raceTime) + "  BEST LAP " +
               Clock(race.bestLap);
    }
    char speed[16];
    std::snprintf(speed, sizeof(speed), "%3d", static_cast<int>(
                                                   pod.speed * 3.6f));
    std::string line = "LAP " + std::to_string(race.lap) + "/" +
                       std::to_string(race.lapsTotal) + "  " +
                       Clock(race.raceTime) + "  BEST " +
                       Clock(race.bestLap) + "  " + speed +
                       " KM/H  HEAT [" + HeatBar(pod.heat) + "]";
    if (pod.overheatTimer > 0.f) line += " ENGINE FIRE";
    else if (pod.boosting) line += " BOOST";
    return line;
}

}  // namespace sdl3cpp::services::impl
