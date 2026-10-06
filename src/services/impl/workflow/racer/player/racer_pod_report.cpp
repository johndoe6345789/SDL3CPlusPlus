#include "services/interfaces/workflow/racer/player/racer_pod_report.hpp"

#include <algorithm>
#include <string>

namespace sdl3cpp::services::impl {

void PublishRacerPod(WorkflowContext& context,
                     const RacerWorldState& state) {
    const RacerPodState& pod = state.pod;
    context.Set("racer.pod_pos", pod.position);
    context.Set("racer.pod_heading", pod.heading);
    context.Set("racer.speed", pod.speed * 3.6f);
    context.Set("racer.heat", pod.heat);
    context.Set("racer.damage", RacerPodDamage(pod));
    context.Set("racer.damage_left", pod.engineDamage[0]);
    context.Set("racer.damage_right", pod.engineDamage[1]);
    context.Set("racer.boosting", pod.boosting);
}

void TraceRacerPod(const std::shared_ptr<ILogger>& logger,
                   const RacerWorldState& state) {
    if (!logger) return;
    const RacerPodState& pod = state.pod;
    const glm::vec3 at = pod.position;
    const auto floor = RacerGroundHeight(state.ground, at.x, at.z, at.y + 3.f);
    const auto top = RacerGroundHeight(state.ground, at.x, at.z, 1e9f);
    const glm::vec3 ahead =
        at + RacerPodForward(pod.heading) * std::max(2.f, pod.speed * 0.05f);
    const int wall = RacerWallHit(state.ground, at, ahead);
    if (wall >= 0) {
        const glm::vec3* w = &state.ground.walls[3 * wall];
        logger->Trace("racer.pod.drive: wall ahead, corners y " +
                      std::to_string(w[0].y) + " " + std::to_string(w[1].y) +
                      " " + std::to_string(w[2].y) + " at x " +
                      std::to_string(w[0].x) + " z " + std::to_string(w[0].z));
    }
    logger->Trace("racer.pod.drive: floor " +
                  (floor ? std::to_string(*floor) : std::string("none")) +
                  ", highest " +
                  (top ? std::to_string(*top) : std::string("none")) +
                  ", ground tris " +
                  std::to_string(state.ground.triangles.size() / 3));
    logger->Trace("racer.pod.drive: at (" + std::to_string(pod.position.x) +
                  ", " + std::to_string(pod.position.y) + ", " +
                  std::to_string(pod.position.z) + ") point " +
                  std::to_string(state.race.segment) + " heading " +
                  std::to_string(pod.heading) + " speed " +
                  std::to_string(pod.speed) +
                  (pod.grounded ? " grounded" : " airborne"));
}

}  // namespace sdl3cpp::services::impl
