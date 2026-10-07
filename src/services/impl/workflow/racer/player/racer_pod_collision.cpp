#include "services/interfaces/workflow/racer/player/racer_pod_physics.hpp"

#include <algorithm>
#include <cmath>

namespace sdl3cpp::services::impl {
namespace {

/// Something within this height above a gap is a wall, not a drop.
constexpr float kWallHeight = 12.f;
constexpr float kScrapeDamage = 0.6f;  // per second at 100 m/s

/// Blocked: a wall stands in the way, or there is no floor at `at`
/// under the step height but something just above it (a ledge).
bool Blocked(const glm::vec3& from, const glm::vec3& at,
             const RacerPodSpec& spec, const RacerSurface& surface) {
    if (surface.wall && surface.wall(surface.context, from, at)) return true;
    const float ceiling = from.y + spec.stepHeight;
    const void* c = surface.context;
    return !surface.height(c, at.x, at.z, ceiling) &&
           surface.height(c, at.x, at.z, ceiling + kWallHeight).has_value();
}

}  // namespace

bool MoveRacerPodAlongSurface(RacerPodState& pod, const RacerPodSpec& spec,
                              float dt, const RacerSurface& surface,
                              glm::vec3& next) {
    // Straight on, then turned ever further aside up to nearly along
    // the wall: a square hit still finds a way to scrape past.
    static const float kAngles[] = {0.f,  0.35f, -0.35f, 0.8f,
                                    -0.8f, 1.3f, -1.3f};
    for (float angle : kAngles) {
        const glm::vec3 step =
            RacerPodForward(pod.heading + angle) * pod.speed * dt;
        if (Blocked(pod.position, pod.position + step, spec, surface)) {
            continue;
        }
        next = pod.position + step;
        if (angle != 0.f) {
            // Scraping the wall: lose speed, turn to run along it, and
            // scuff the engine on the wall's side (turning right to get
            // clear means the wall was on the left).
            pod.speed *= 1.f - 0.08f * std::fabs(angle);
            pod.heading += 0.5f * angle;
            const int side = angle > 0.f ? 0 : 1;
            const float hit = kScrapeDamage * std::fabs(pod.speed) / 100.f *
                              (1.5f - spec.damageImmunity);
            pod.engineDamage[side] =
                std::min(1.f, pod.engineDamage[side] + hit * dt);
        }
        return true;
    }
    pod.speed *= 0.5f;
    return false;
}

}  // namespace sdl3cpp::services::impl
