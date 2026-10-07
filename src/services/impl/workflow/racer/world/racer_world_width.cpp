#include "services/interfaces/workflow/racer/world/racer_world_build.hpp"

#include <algorithm>
#include <cmath>
#include <string>

namespace sdl3cpp::services::impl {
namespace {

/// Metres of floor from `from` along `side` before it ends or jumps.
float FloorReach(const RacerGround& ground, glm::vec3 from,
                 const glm::vec3& side) {
    constexpr float kStep = 0.5f;
    float reach = 0.f;
    for (; reach < 400.f; reach += kStep) {
        const glm::vec3 p = from + side * (reach + kStep);
        const auto floor = RacerGroundHeight(ground, p.x, p.z, from.y + 6.f);
        if (!floor || std::fabs(*floor - from.y) > 3.f) break;
        from.y = *floor;
    }
    return reach;
}

}  // namespace

void TraceRacerTrackWidth(const std::shared_ptr<ILogger>& logger,
                          const RacerWorldState& state) {
    if (!logger || state.lapPoints.size() < 2) return;
    std::vector<float> widths;
    const std::size_t count = state.lapPoints.size();
    for (std::size_t i = 0; i < count; i += 25) {
        const glm::vec3 a = state.lapPoints[i];
        glm::vec3 along = state.lapPoints[(i + 1) % count] - a;
        along.y = 0.f;
        if (glm::length(along) < 1e-4f) continue;
        const glm::vec3 side =
            glm::normalize(glm::vec3(-along.z, 0.f, along.x));
        const auto floor = RacerGroundHeight(state.ground, a.x, a.z,
                                             a.y + 30.f);
        if (!floor) continue;
        const glm::vec3 on(a.x, *floor, a.z);
        widths.push_back(FloorReach(state.ground, on, side) +
                         FloorReach(state.ground, on, -side));
    }
    // Stretches of the line with no floor near it: collision missing.
    std::string gaps;
    int missing = 0;
    for (std::size_t i = 0; i < count; ++i) {
        const glm::vec3 p = state.lapPoints[i];
        const auto floor =
            RacerGroundHeight(state.ground, p.x, p.z, p.y + 10.f);
        const bool none = !floor || *floor < p.y - 4.f;
        missing += none ? 1 : 0;
        const bool startRun = none && (i == 0 || gaps.empty() ||
                                       gaps.back() == ' ');
        if (none && startRun) gaps += std::to_string(i) + "-";
        if (!none && !gaps.empty() && gaps.back() == '-') {
            gaps += std::to_string(i - 1) + " ";
        }
    }
    logger->Trace("racer.world.load: " + std::to_string(missing) + " of " +
                  std::to_string(count) + " line points have no floor: " +
                  gaps.substr(0, 400));
    if (widths.empty()) return;
    std::sort(widths.begin(), widths.end());
    logger->Trace("racer.world.load: track width median " +
                  std::to_string(widths[widths.size() / 2]) +
                  " m, narrowest tenth " +
                  std::to_string(widths[widths.size() / 10]) + " m");
}

}  // namespace sdl3cpp::services::impl
