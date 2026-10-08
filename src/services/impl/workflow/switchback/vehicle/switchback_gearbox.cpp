#include "services/interfaces/workflow/switchback/vehicle/switchback_gearbox.hpp"

#include <algorithm>

namespace sdl3cpp::services::impl {
namespace {

constexpr int kTopGear = 8;
// Speed at the redline in each gear, m/s. Each gear is about 1.3 times the
// last; eighth tops out near 309 mph.
constexpr float kRedlineSpeed[kTopGear] = {22.f, 29.f, 38.f, 49.f,
                                           63.f, 82.f, 106.f, 138.f};
constexpr float kUpshiftRevs = 0.97f;
constexpr float kDownshiftRevs = 0.35f;
// Under throttle at the redline the box changes up, but not before the
// wheels turn at this share of the gear's redline speed (no shifting on
// the line while the clutch is dumped).
constexpr float kRedlineShiftWheelRevs = 0.4f;
// Above this wheel speed the clutch is locked and the engine turns with the
// wheels. Below it the clutch slips and the engine revs freely: a launch.
constexpr float kClutchLockRevs = 0.35f;
// Revs per second the engine climbs under full throttle, and falls off it.
constexpr float kRiseRate = 1.2f;
constexpr float kFallRate = 1.5f;

// The engine's revs: the wheels' revs, or the engine's own revs where the
// clutch slips.
float BlendRevs(float wheelRevs, float freeRevs) {
    const float slip = 1.f - std::clamp(wheelRevs / kClutchLockRevs, 0.f, 1.f);
    const float blended = wheelRevs + (freeRevs - wheelRevs) * slip;
    return std::clamp(std::max(wheelRevs, blended), 0.f, 1.f);
}

}  // namespace

void SwitchbackGearbox::Update(float speed, float throttle, float dt) {
    const float s = std::max(0.f, speed);
    const int previousGear = gear_;
    while (gear_ > 1 && s < kDownshiftRevs * kRedlineSpeed[gear_ - 1]) {
        --gear_;
    }
    while (gear_ < kTopGear &&
           s >= kUpshiftRevs * kRedlineSpeed[gear_ - 1]) {
        ++gear_;
    }
    if (gear_ != previousGear) freeRevs_ = s / kRedlineSpeed[gear_ - 1];
    if (throttle > 0.f) {
        freeRevs_ = std::min(1.f, freeRevs_ + throttle * kRiseRate * dt);
    } else {
        freeRevs_ = std::max(0.f, freeRevs_ - kFallRate * dt);
    }
    float wheelRevs = s / kRedlineSpeed[gear_ - 1];
    if (throttle > 0.f && gear_ < kTopGear &&
        wheelRevs >= kRedlineShiftWheelRevs &&
        BlendRevs(wheelRevs, freeRevs_) >= kUpshiftRevs) {
        ++gear_;
        wheelRevs = s / kRedlineSpeed[gear_ - 1];
        freeRevs_ = wheelRevs;
    }
    revs_ = BlendRevs(wheelRevs, freeRevs_);
    const bool clutchLocked = wheelRevs >= kClutchLockRevs;
    powerScale_ = (clutchLocked && revs_ >= 1.f) ? 0.f : 1.f;
}

}  // namespace sdl3cpp::services::impl
