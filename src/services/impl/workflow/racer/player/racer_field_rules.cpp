#include "services/interfaces/workflow/racer/player/racer_field_rules.hpp"

#include <algorithm>

namespace sdl3cpp::services::impl {
namespace {

constexpr float kMaxPush = 0.25f;   // metres per contact per frame
// Above any racing progress (laps x points), small enough that a float
// still resolves hundredths of a second when the race time is taken off.
constexpr float kFinishedBonus = 1e5f;

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
    // A pod is long (engines, cables, cockpit): three circles down its
    // length stand in for its outline. The lighter pod gives way more;
    // deep overlaps resolve over a few frames.
    for (std::size_t i = 0; i < pods.size(); ++i) {
        for (std::size_t j = i + 1; j < pods.size(); ++j) {
            RacerPodState& a = *pods[i];
            RacerPodState& b = *pods[j];
            const float reach = a.bodyHalfWidth + b.bodyHalfWidth;
            const float share = b.mass / std::max(1.f, a.mass + b.mass);
            for (const glm::vec3& ca : RacerBodyCircles(a)) {
                for (const glm::vec3& cb : RacerBodyCircles(b)) {
                    glm::vec3 d = cb - ca;
                    d.y = 0.f;
                    const float distance = glm::length(d);
                    if (distance >= reach || distance < 1e-4f) continue;
                    const float depth =
                        std::min(kMaxPush, 0.5f * (reach - distance));
                    const glm::vec3 push = d / distance * (2.f * depth);
                    a.bumped = b.bumped = true;
                    a.position -= push * share;
                    b.position += push * (1.f - share);
                    const float shared = 0.5f * (a.speed + b.speed);
                    a.speed = 0.9f * a.speed + 0.1f * shared;
                    b.speed = 0.9f * b.speed + 0.1f * shared;
                }
            }
        }
    }
}

std::array<glm::vec3, 3> RacerBodyCircles(const RacerPodState& pod) {
    const glm::vec3 f = RacerPodForward(pod.heading);
    return {pod.position + f * (0.5f * pod.bodyFront),
            pod.position - f * (0.4f * pod.bodyBack),
            pod.position - f * (0.85f * pod.bodyBack)};
}

}  // namespace sdl3cpp::services::impl
