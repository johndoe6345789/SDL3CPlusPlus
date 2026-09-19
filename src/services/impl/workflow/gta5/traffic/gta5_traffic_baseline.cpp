#include "services/interfaces/workflow/gta5/traffic/gta5_traffic_ai.hpp"

#include <algorithm>
#include <cmath>

namespace sdl3cpp::services::impl {

void Gta5DriverInputs(const Gta5DriverSense& sense, float* out) {
    out[0] = sense.speed / 20.f;
    out[1] = sense.off / 3.14159265f;
    out[2] = sense.turning / 2.f;
    out[3] = std::min(sense.gap, 40.f) / 40.f;
    out[4] = std::min(sense.line, 40.f) / 40.f;
    out[5] = sense.want / 20.f;
    out[6] = sense.follow / 10.f;
    out[7] = sense.nerve;
}

Gta5DriverAction Gta5BaselineDriver(const Gta5DriverSense& s) {
    float want = s.want;
    // Off the throttle for whatever is in front, and stopped well
    // before running into it.
    if (s.gap < s.follow + 6.f) {
        want = std::min(want, std::max(0.f, s.gap - s.follow) * 1.6f);
    }
    // Held at the line when the light is against this arm.
    if (s.line < 22.f) {
        want = std::min(want, std::max(0.f, s.line - 7.f) * 1.1f);
    }
    // Slow into a bend rather than understeering through it; a bold
    // driver slows less.
    want *= std::max(0.4f, 1.f - std::fabs(s.off) * 0.6f / s.nerve);
    // A softer hand the faster it goes, and damped by how fast it is
    // already turning, or it weaves down a straight road.
    const float gain = 1.6f / (1.f + s.speed * 0.12f);
    Gta5DriverAction out;
    out.steer = std::clamp(s.off * gain - s.turning * 0.25f, -1.f, 1.f);
    const float pedal = std::clamp((want - s.speed) * 0.5f, -1.f, 1.f);
    out.throttle = std::max(0.f, pedal);
    out.brake = (want < 0.5f && s.speed > 0.6f) ? 0.8f
                : (pedal < -0.4f ? 0.35f : 0.f);
    return out;
}

}  // namespace sdl3cpp::services::impl
