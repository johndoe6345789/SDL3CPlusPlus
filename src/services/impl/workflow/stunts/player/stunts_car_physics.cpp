#include "services/interfaces/workflow/stunts/player/stunts_car_physics.hpp"

#include <algorithm>
#include <cmath>

namespace sdl3cpp::services::impl {
namespace {

constexpr float kMinGearSpan = 0.05f;

/// Share of the top gear's speed each gear reaches, so that the box
/// spreads the rev range evenly over the car's own gear count.
float GearCeiling(int gear, int gears) {
    const int count = std::max(gears, 1);
    const float step = 1.f / static_cast<float>(count);
    return std::clamp(step * static_cast<float>(gear), kMinGearSpan, 1.f);
}

}  // namespace

StuntsDriveTuning StuntsTuningFor(const StuntsEngine& engine) {
    StuntsDriveTuning tune;
    // `performance` tracks the figures the showroom prints closely
    // enough to scale from: it rises with quoted top speed across the
    // stock cars, so it sets both the ceiling and the urge to reach it.
    const float index = std::clamp(static_cast<float>(engine.performance),
                                   16.f, 48.f);
    tune.topSpeed = 40.f + index;
    tune.drive = 4.f + index * 0.12f;
    return tune;
}

float StuntsRpmFor(const StuntsEngine& engine, const StuntsDriveTuning& tune,
                   float speed, int gear) {
    const float ceiling = GearCeiling(gear, engine.gears) * tune.topSpeed;
    const float share = ceiling > 0.f ? std::fabs(speed) / ceiling : 0.f;
    const float idle = static_cast<float>(engine.idleRpm);
    const float limit = static_cast<float>(engine.revLimit);
    return std::clamp(idle + share * (limit - idle), idle, limit);
}

int StuntsGearFor(const StuntsEngine& engine, const StuntsDriveTuning& tune,
                  float speed, int gear) {
    const int top = std::max(engine.gears, 1);
    int chosen = std::clamp(gear, 1, top);
    if (StuntsRpmFor(engine, tune, speed, chosen) >=
            static_cast<float>(engine.redLine) && chosen < top) {
        ++chosen;
    }
    while (chosen > 1 &&
           StuntsRpmFor(engine, tune, speed, chosen) <=
               static_cast<float>(engine.idleRpm) * 1.3f) {
        --chosen;
    }
    return chosen;
}

}  // namespace sdl3cpp::services::impl
