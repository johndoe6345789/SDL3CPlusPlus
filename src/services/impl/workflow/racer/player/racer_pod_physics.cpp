#include "services/interfaces/workflow/racer/player/racer_pod_physics.hpp"

#include <algorithm>
#include <cmath>

namespace sdl3cpp::services::impl {
namespace {

/// Something within this height above a gap is a wall, not a drop.
constexpr float kWallHeight = 12.f;

}  // namespace

glm::vec3 RacerPodForward(float heading) {
    return {std::sin(heading), 0.f, -std::cos(heading)};
}

void StepRacerPod(RacerPodState& pod, const RacerPodInput& in,
                  const RacerPodSpec& spec, float dt, RacerGroundProbe probe,
                  const void* ground) {
    if (dt <= 0.f) return;
    UpdateRacerPodEngines(pod, in, spec, dt);
    const float target = RacerPodTargetSpeed(pod, in, spec);
    if (in.throttle < -0.1f) {
        pod.speed -= spec.brakeDeceleration * -in.throttle * dt;
        pod.speed = std::max(pod.speed, -0.15f * spec.topSpeed);
    } else if (pod.speed < target) {
        const float boost = pod.boosting ? 1.6f : 1.f;
        pod.speed =
            std::min(target, pod.speed + spec.acceleration * boost * dt);
    } else {
        pod.speed = std::max(target, pod.speed - pod.speed * spec.drag * dt);
    }
    // Pods turn tighter when slow; at full tilt the turn rate halves.
    const float grip = 1.f - 0.5f * std::min(1.f, pod.speed / spec.topSpeed);
    pod.heading += in.steer * spec.turnRate * grip * dt;

    const glm::vec3 step = RacerPodForward(pod.heading) * pod.speed * dt;
    glm::vec3 next = pod.position + step;
    const float ceiling = pod.position.y + spec.stepHeight;
    // No floor ahead but something just above it: the pod has met a
    // wall or a ledge, so it stops and loses half its speed.
    if (!probe(ground, next.x, next.z, ceiling) &&
        probe(ground, next.x, next.z, ceiling + kWallHeight)) {
        next = pod.position;
        pod.speed *= 0.5f;
    }
    pod.verticalSpeed -= spec.gravity * dt;
    next.y = pod.position.y + pod.verticalSpeed * dt;
    const auto floor = probe(ground, next.x, next.z, ceiling);
    pod.grounded = floor && next.y <= *floor + spec.hoverHeight;
    if (pod.grounded) {
        next.y = *floor + spec.hoverHeight;
        pod.verticalSpeed = std::max(0.f, pod.verticalSpeed);
        pod.airTime = 0.f;
    } else {
        pod.airTime += dt;
    }
    pod.position = next;
}

}  // namespace sdl3cpp::services::impl
