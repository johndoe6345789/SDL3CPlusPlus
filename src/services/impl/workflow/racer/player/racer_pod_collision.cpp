#include "services/interfaces/workflow/racer/player/racer_pod_physics.hpp"

#include <cmath>

namespace sdl3cpp::services::impl {
namespace {

/// Something within this height above a gap is a wall, not a drop.
constexpr float kWallHeight = 12.f;

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
            // Scraping the wall: lose speed, and turn to run along it.
            pod.speed *= 1.f - 0.08f * std::fabs(angle);
            pod.heading += 0.5f * angle;
        }
        return true;
    }
    pod.speed *= 0.5f;
    return false;
}

}  // namespace sdl3cpp::services::impl
