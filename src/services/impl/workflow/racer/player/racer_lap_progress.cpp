#include "services/interfaces/workflow/racer/player/racer_lap_progress.hpp"

#include <limits>

namespace sdl3cpp::services::impl {
namespace {

constexpr int kSearchWindow = 6;

}  // namespace

int NearestRacerLapPoint(const std::vector<glm::vec3>& points,
                         const glm::vec3& position, int previous) {
    const int count = static_cast<int>(points.size());
    if (count == 0) return -1;
    const bool local = previous >= 0 && count > 2 * kSearchWindow;
    const int from = local ? previous - kSearchWindow : 0;
    const int to = local ? previous + kSearchWindow : count - 1;
    int best = -1;
    float bestDistance = std::numeric_limits<float>::max();
    for (int i = from; i <= to; ++i) {
        const int index = ((i % count) + count) % count;
        const glm::vec3 d = points[index] - position;
        const float distance = glm::dot(d, d);
        if (distance < bestDistance) {
            bestDistance = distance;
            best = index;
        }
    }
    return best;
}

void AdvanceRacerRace(RacerRaceState& race, int point, int pointCount,
                      float dt) {
    if (race.countdown > 0.f) {
        race.countdown -= dt;
        return;
    }
    if (race.finished) return;
    race.raceTime += dt;
    race.lapTime += dt;
    const int quarter = pointCount / 4;
    const bool wrapped = race.segment >= pointCount - quarter &&
                         point >= 0 && point < quarter;
    if (wrapped) {
        if (race.bestLap <= 0.f || race.lapTime < race.bestLap) {
            race.bestLap = race.lapTime;
        }
        race.lapTime = 0.f;
        ++race.lap;
        race.finished = race.lap > race.lapsTotal;
    }
    race.segment = point;
}

}  // namespace sdl3cpp::services::impl
