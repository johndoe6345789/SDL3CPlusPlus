#include "services/interfaces/workflow/racer/flow/racer_flow_rules.hpp"

#include <algorithm>

namespace sdl3cpp::services::impl {
namespace {

constexpr float kPerLevel = 0.05f;

}  // namespace

const char* RacerUpgradeName(int index) {
    static const char* const kNames[kRacerUpgradeCount] = {
        "TRACTION", "TURNING", "ACCELERATION", "TOP SPEED",
        "AIR BRAKE", "COOLING", "REPAIR"};
    return index >= 0 && index < kRacerUpgradeCount ? kNames[index] : "";
}

int RacerUpgradeCost(int level) {
    return 250 * (level + 1);
}

int RacerPrizeFor(int position) {
    static const int kPrizes[] = {1500, 900, 600, 350, 200, 120, 80, 50};
    if (position < 1) return 0;
    return position <= 8 ? kPrizes[position - 1] : 30;
}

RacerPodSpec ApplyRacerUpgrades(RacerPodSpec spec,
                                const RacerProfile& profile) {
    auto gain = [&profile](int part) {
        return 1.f + kPerLevel * static_cast<float>(profile.upgrades[part]);
    };
    // Traction keeps more of the turn rate at speed.
    spec.turnAtTopSpeed = std::min(0.95f, spec.turnAtTopSpeed * gain(0));
    spec.turnRate *= gain(1);
    spec.acceleration *= gain(2);
    spec.topSpeed *= gain(3);
    spec.boostSpeed *= gain(3);
    spec.brakeDeceleration *= gain(4);
    spec.coolRate *= gain(5);
    spec.heatRate /= gain(5);
    spec.repairRate *= gain(6);
    return spec;
}

}  // namespace sdl3cpp::services::impl
