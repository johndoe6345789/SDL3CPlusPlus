#include "services/interfaces/workflow/racer/world/racer_world_build.hpp"

#include <string>

namespace sdl3cpp::services::impl {

void BuildRacerLapPoints(RacerWorldState& state) {
    // The lap as a dense polyline along the Bezier curves: the knots
    // alone are ~30 m apart, too coarse to steer by on narrow sections.
    state.lapPoints.clear();
    const std::size_t count = state.lap.size();
    for (std::size_t i = 0; i < count; ++i) {
        const auto& from = state.spline[state.lap[i]];
        const auto& to = state.spline[state.lap[(i + 1) % count]];
        for (int k = 0; k < kRacerLapSamples; ++k) {
            const RacerVec3 p = RacerSplinePoint(
                from, to, static_cast<float>(k) / kRacerLapSamples);
            state.lapPoints.push_back(RacerToEngine(p.x, p.y, p.z));
        }
    }
}

bool BuildRacerCollisionGround(RacerWorldState& state,
                               const RacerModel& track) {
    // Pods ride the invisible collision surface the game uses, which
    // includes walls the visible meshes do not have.
    const bool collision = !track.collision.empty();
    for (std::size_t i = 0; collision && i + 8 < track.collision.size();
         i += 9) {
        const float* c = &track.collision[i];
        AddRacerGroundTriangle(state.ground, RacerToEngine(c[0], c[1], c[2]),
                               RacerToEngine(c[3], c[4], c[5]),
                               RacerToEngine(c[6], c[7], c[8]));
    }
    return collision;
}

/// Trace: a model's extent in engine units, for checking scale.
void TraceModelExtent(const std::shared_ptr<ILogger>& logger,
                      const std::string& what, const RacerModel& model) {
    if (!logger) return;
    glm::vec3 lo(1e9f), hi(-1e9f);
    for (const auto& batch : model.batches) {
        for (const auto& v : batch.vertices) {
            const glm::vec3 p = RacerToEngine(v.x, v.y, v.z);
            lo = glm::min(lo, p);
            hi = glm::max(hi, p);
        }
    }
    logger->Trace("racer.world.load: " + what + " extent x " +
                  std::to_string(lo.x) + ".." + std::to_string(hi.x) +
                  " y " + std::to_string(lo.y) + ".." + std::to_string(hi.y) +
                  " z " + std::to_string(lo.z) + ".." + std::to_string(hi.z));
}

}  // namespace sdl3cpp::services::impl
