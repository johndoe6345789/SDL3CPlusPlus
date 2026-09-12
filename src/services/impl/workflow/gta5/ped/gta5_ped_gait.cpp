#include "services/interfaces/workflow/gta5/ped/gta5_ped_gait.hpp"

#include <algorithm>

namespace sdl3cpp::services::impl {
namespace {

/// A stroll: short steps, a straight back, arms hanging and swinging
/// from the shoulder rather than driving.
constexpr Gta5Gait kWalk{1.35f, 22.f, 55.f, 18.f, 16.f, 20.f,
                         2.f,   6.f,  0.035f};

/// A hard run: long steps, the knee folded right up, the chest out over
/// the hips and the arms working.
constexpr Gta5Gait kRun{2.30f, 40.f, 95.f, 26.f, 44.f, 78.f,
                        11.f,  13.f, 0.022f};

float Mix(float from, float to, float t) { return from + (to - from) * t; }

}  // namespace

Gta5Gait Gta5GaitFor(float speed) {
    // Under a stroll it is all walk and over a hard run all run; a jog
    // is the two mixed, which is what keeps the legs under the ground
    // speed instead of skating or sprinting on the spot.
    const float t = std::clamp((speed - 1.6f) / 4.4f, 0.f, 1.f);
    Gta5Gait out;
    out.stride = Mix(kWalk.stride, kRun.stride, t);
    out.thigh = Mix(kWalk.thigh, kRun.thigh, t);
    out.knee = Mix(kWalk.knee, kRun.knee, t);
    out.ankle = Mix(kWalk.ankle, kRun.ankle, t);
    out.arm = Mix(kWalk.arm, kRun.arm, t);
    out.elbow = Mix(kWalk.elbow, kRun.elbow, t);
    out.lean = Mix(kWalk.lean, kRun.lean, t);
    out.twist = Mix(kWalk.twist, kRun.twist, t);
    out.sway = Mix(kWalk.sway, kRun.sway, t);
    return out;
}

}  // namespace sdl3cpp::services::impl
