#include "services/interfaces/workflow/racer/player/racer_autopilot.hpp"

#include "services/interfaces/workflow/racer/render/racer_camera_math.hpp"

#include <algorithm>
#include <array>
#include <cmath>

namespace sdl3cpp::services::impl {
namespace {

constexpr float kStep = 8.f;      // metres between checks along the lap
constexpr int kWindow = 3;        // checks a bend is measured across
constexpr float kMargin = 0.72f;  // corner speeds taken with room spare

float Tangent(const std::vector<glm::vec3>& lap, int i) {
    const int count = static_cast<int>(lap.size());
    const glm::vec3 d = lap[(i + 1) % count] - lap[i];
    return std::atan2(d.x, -d.z);
}

}  // namespace

float RacerCornerSpeed(const RacerPodSpec& spec, float radius) {
    // Turn rate falls with speed: w(v) = w0 (1 - (1 - k) v / top).
    // Holding radius r needs v <= r w(v), so v <= r w0 / (1 + r w0 c).
    const float w0 = spec.turnRate;
    const float c = (1.f - spec.turnAtTopSpeed) / spec.topSpeed;
    return radius * w0 / (1.f + radius * w0 * c);
}

float RacerPlannedSpeed(const std::vector<glm::vec3>& lapPoints,
                        int nearestPoint, const RacerPodSpec& spec,
                        float horizon) {
    const int count = static_cast<int>(lapPoints.size());
    float planned = spec.boostSpeed;
    if (count < 3 || nearestPoint < 0) return planned;
    // Walk the lap; wherever its direction bends, the bend over the
    // last stretch gives a radius, its corner speed a limit, and the
    // air brake how fast the pod may arrive from here.
    int i = nearestPoint % count;
    float run = 0.f, sinceCheck = 0.f;
    // Headings at the last few checks: a bend is measured over the
    // whole window, as single steps of the spline are too noisy.
    std::array<float, kWindow> headings;
    headings.fill(Tangent(lapPoints, i));
    int check = 0;
    while (run < horizon) {
        const int next = (i + 1) % count;
        const float step = glm::length(lapPoints[next] - lapPoints[i]);
        run += step;
        sinceCheck += step;
        i = next;
        if (sinceCheck < kStep) continue;
        const float heading = Tangent(lapPoints, i);
        float& oldest = headings[check++ % kWindow];
        const float bend = std::fabs(RacerAngleDelta(oldest, heading));
        const float radius = kWindow * kStep / std::max(bend, 1e-3f);
        oldest = heading;
        const float corner = kMargin * RacerCornerSpeed(spec, radius);
        const float brake = 0.6f * spec.brakeDeceleration;
        const float arrive = std::sqrt(corner * corner + 2.f * brake * run);
        planned = std::min(planned, arrive);
        sinceCheck = 0.f;
    }
    return std::max(planned, 0.25f * spec.topSpeed);
}

}  // namespace sdl3cpp::services::impl
