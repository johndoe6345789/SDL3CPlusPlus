#include "services/interfaces/workflow/racer/player/racer_hazards.hpp"

#include <cmath>

namespace sdl3cpp::services::impl {
namespace {

// Where round the lap hazards sit, clear of the start.
constexpr float kSpots[] = {0.2f, 0.45f, 0.7f};
constexpr float kPerchSide = 28.f;   // metres from the line, Tuskens
constexpr float kPerchHeight = 14.f;

bool KindFor(const std::string& planet, RacerHazardKind& kind) {
    if (planet == "Tatooine") {
        kind = RacerHazardKind::Blaster;
    } else if (planet == "Baroonda" || planet == "Malastare") {
        kind = RacerHazardKind::Eruption;
    } else if (planet == "Ando Prime" || planet == "Ord Ibanna" ||
               planet == "Mon Gazza") {
        kind = RacerHazardKind::Rockfall;
    } else {
        return false;
    }
    return true;
}

}  // namespace

std::vector<RacerHazard> PlaceRacerHazards(
    const std::string& planet, const std::vector<glm::vec3>& lap) {
    std::vector<RacerHazard> hazards;
    RacerHazardKind kind;
    if (lap.size() < 16 || !KindFor(planet, kind)) return hazards;
    std::uint32_t seed = 1;
    for (const float spot : kSpots) {
        const auto i = static_cast<std::size_t>(spot * lap.size());
        const glm::vec3 along = lap[(i + 1) % lap.size()] - lap[i];
        glm::vec3 side(-along.z, 0.f, along.x);
        side = glm::length(side) > 1e-4f ? glm::normalize(side)
                                         : glm::vec3(1.f, 0.f, 0.f);
        const float flip = (seed % 2 == 0) ? 1.f : -1.f;
        RacerHazard h;
        h.kind = kind;
        h.seed = seed;
        h.at = lap[i];
        if (kind == RacerHazardKind::Blaster) {
            h.at += side * (flip * kPerchSide) +
                    glm::vec3(0.f, kPerchHeight, 0.f);
            h.radius = 110.f;   // a Tusken's range
            h.period = 1.4f;
        } else if (kind == RacerHazardKind::Eruption) {
            h.at += side * (flip * 3.f);
            h.radius = 7.f;
            h.period = 7.f;
        } else {
            h.radius = 6.f;
            h.period = 9.f;
        }
        h.clock = 0.37f * h.period * static_cast<float>(seed);
        hazards.push_back(h);
        seed += 1;
    }
    return hazards;
}

}  // namespace sdl3cpp::services::impl
