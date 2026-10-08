#include "services/interfaces/workflow/switchback/vehicle/switchback_gearbox.hpp"

#include <algorithm>

namespace sdl3cpp::services::impl {
namespace {

constexpr int kTopGear = 6;
// Speed at the redline in each gear, m/s.
constexpr float kRedlineSpeed[kTopGear] = {22.f, 42.f, 66.f, 95.f, 120.f,
                                           140.f};
constexpr float kIdleRevs = 0.1f;
constexpr float kUpshiftRevs = 0.97f;
constexpr float kDownshiftRevs = 0.35f;

}  // namespace

void SwitchbackGearbox::Update(float speed) {
    const float s = std::max(0.f, speed);
    while (gear_ < kTopGear &&
           s >= kUpshiftRevs * kRedlineSpeed[gear_ - 1]) {
        ++gear_;
    }
    while (gear_ > 1 && s < kDownshiftRevs * kRedlineSpeed[gear_ - 1]) {
        --gear_;
    }
    const float fraction = s / kRedlineSpeed[gear_ - 1];
    revs_ = std::clamp(fraction, kIdleRevs, 1.f);
}

}  // namespace sdl3cpp::services::impl
