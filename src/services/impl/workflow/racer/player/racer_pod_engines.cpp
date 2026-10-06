#include "services/interfaces/workflow/racer/player/racer_pod_physics.hpp"

#include "services/interfaces/workflow/racer/player/racer_surface_effects.hpp"

#include <algorithm>

namespace sdl3cpp::services::impl {

namespace {

constexpr float kOverheatSeconds = 3.f;

}  // namespace

/// Boost needs near-top speed and a cool engine, as in the game, and
/// heats the engines until they blow (then they are damaged).
void UpdateRacerPodEngines(RacerPodState& pod, const RacerPodInput& in,
                           const RacerPodSpec& spec, float dt) {
    pod.overheatTimer = std::max(0.f, pod.overheatTimer - dt);
    pod.boosting = in.boost && in.throttle > 0.5f &&
                   pod.overheatTimer <= 0.f &&
                   pod.speed > 0.85f * spec.topSpeed;
    if (pod.boosting) {
        pod.heat += spec.heatRate * dt;
        if (pod.heat >= 1.f) {
            pod.heat = 1.f;
            pod.overheatTimer = kOverheatSeconds;
            // An engine fire hurts both engines, one a little more.
            pod.engineDamage[0] = std::min(1.f, pod.engineDamage[0] + 0.35f);
            pod.engineDamage[1] = std::min(1.f, pod.engineDamage[1] + 0.3f);
            pod.boosting = false;
        }
    } else {
        pod.heat = std::max(0.f, pod.heat - spec.coolRate * dt);
    }
    // Lava heats the engines whether or not they are boosting.
    pod.heat = std::min(
        1.f, pod.heat + RacerSurfaceEffectFor(pod.surface).heatPerSecond * dt);
    if (in.repair) {
        for (float& damage : pod.engineDamage) {
            damage = std::max(0.f, damage - spec.repairRate * dt);
        }
    }
}

float RacerPodTargetSpeed(const RacerPodState& pod,
                          const RacerPodInput& in,
                          const RacerPodSpec& spec) {
    float top = pod.boosting ? spec.boostSpeed : spec.topSpeed;
    top *= 1.f - 0.4f * RacerPodDamage(pod);
    top *= RacerSurfaceEffectFor(pod.surface).topSpeed;
    if (in.repair) top *= 0.6f;  // repairing costs speed
    return top * std::clamp(in.throttle, 0.f, 1.f);
}

}  // namespace sdl3cpp::services::impl
