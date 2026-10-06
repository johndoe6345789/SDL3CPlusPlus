#include "services/interfaces/workflow/racer/player/racer_field_rules.hpp"

#include <glm/glm.hpp>

#include <algorithm>

namespace sdl3cpp::services::impl {
namespace {

constexpr float kPodRadius = 2.6f;
constexpr float kFinishedBonus = 1e7f;

}  // namespace

float RacerRaceProgress(const RacerRaceState& race, int lapPointCount) {
    // A finisher outranks anyone still racing; earlier finishers first.
    if (race.finished) return kFinishedBonus - race.raceTime;
    return static_cast<float>((race.lap - 1) * lapPointCount +
                              std::max(0, race.segment));
}

void RankRacerField(RacerWorldState& state) {
    const int count = static_cast<int>(state.lapPoints.size());
    std::vector<RacerRaceState*> races{&state.race};
    for (RacerOpponent& opponent : state.opponents) {
        races.push_back(&opponent.race);
    }
    for (RacerRaceState* race : races) {
        const float mine = RacerRaceProgress(*race, count);
        int ahead = 0;
        for (const RacerRaceState* other : races) {
            if (other != race && RacerRaceProgress(*other, count) > mine) {
                ++ahead;
            }
        }
        race->position = ahead + 1;
        race->entrants = static_cast<int>(races.size());
    }
}

void SeparateRacerPods(const std::vector<RacerPodState*>& pods) {
    for (std::size_t i = 0; i < pods.size(); ++i) {
        for (std::size_t j = i + 1; j < pods.size(); ++j) {
            glm::vec3 d = pods[j]->position - pods[i]->position;
            d.y = 0.f;
            const float distance = glm::length(d);
            if (distance >= 2.f * kPodRadius || distance < 1e-4f) continue;
            const glm::vec3 push =
                d / distance * (0.5f * (2.f * kPodRadius - distance));
            pods[i]->position -= push;
            pods[j]->position += push;
            const float shared = 0.5f * (pods[i]->speed + pods[j]->speed);
            pods[i]->speed = 0.8f * pods[i]->speed + 0.2f * shared;
            pods[j]->speed = 0.8f * pods[j]->speed + 0.2f * shared;
        }
    }
}

}  // namespace sdl3cpp::services::impl
