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

/// An engine's health as a short bar: '=' per fifth left.
std::string Health(float damage) {
    const int left = static_cast<int>(std::lround((1.f - damage) * 5.f));
    return "[" + std::string(left, '=') + std::string(5 - left, ' ') + "]";
}

/// "POS 3/8  " once the field has been ranked, else nothing.
std::string Place(const RacerRaceState& race) {
    if (race.position <= 0 || race.entrants <= 1) return "";
    return "POS " + std::to_string(race.position) + "/" +
           std::to_string(race.entrants) + "  ";
}

}  // namespace

std::string FormatRacerHud(const RacerRaceState& race,
                           const RacerPodState& pod) {
    const std::string newline(1, '\n');
    if (race.countdown > 0.f) {
        const int second = static_cast<int>(std::ceil(race.countdown));
        return newline + "         " + std::to_string(second);
    }
    const std::string lap = race.finished
                                ? std::string("FINISHED")
                                : "LAP " + std::to_string(race.lap) + "/" +
                                      std::to_string(race.lapsTotal);
    char speed[24];
    std::snprintf(speed, sizeof(speed), "%3d KM/H",
                  static_cast<int>(pod.speed * 3.6f));
    std::string engines = "HEAT [" + HeatBar(pod.heat) + "]";
    if (pod.overheatTimer > 0.f) engines += " ENGINE FIRE";
    else if (pod.boosting) engines += " BOOST";
    return Place(race) + lap + newline + "TIME " + Clock(race.raceTime) +
           "  BEST " + Clock(race.bestLap) + newline + speed + newline +
           engines + newline + "ENGINES " + Health(pod.engineDamage[0]) +
           " " + Health(pod.engineDamage[1]);
}

}  // namespace sdl3cpp::services::impl
