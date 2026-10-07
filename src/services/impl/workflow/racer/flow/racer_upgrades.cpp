#include "services/interfaces/workflow/racer/flow/racer_flow_rules.hpp"

#include "services/interfaces/workflow/racer/flow/racer_parts.hpp"

#include <algorithm>

namespace sdl3cpp::services::impl {
namespace {

constexpr float kPerLevel = 0.05f;

}  // namespace

RacerPodSpec ApplyRacerUpgrades(RacerPodSpec spec,
                                const RacerProfile& profile) {
    // A worn part gives only part of its improvement; a broken one none.
    auto gain = [&profile](int part) {
        const float level = static_cast<float>(profile.upgrades[part]) *
                            std::clamp(profile.health[part], 0.f, 1.f);
        return 1.f + kPerLevel * level;
    };
    // Traction keeps more of the turn rate at speed, and grip on ice.
    spec.turnAtTopSpeed = std::min(0.95f, spec.turnAtTopSpeed * gain(0));
    spec.antiSkid = std::min(1.f, spec.antiSkid * gain(0));
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

bool FitRacerPart(RacerFlow& flow, int type, int level, float health,
                  int price) {
    RacerProfile& profile = flow.profile;
    const int tradeIn = RacerTradeInValue(type, profile.upgrades[type],
                                          profile.health[type]);
    if (profile.truguts + tradeIn < price) {
        flow.notice = "NOT ENOUGH TRUGUTS - WIN SOME RACES";
        return false;
    }
    profile.truguts += tradeIn - price;
    profile.upgrades[type] = level;
    profile.health[type] = health;
    flow.notice = std::string("FITTED ") + RacerPart(type, level).name +
                  (tradeIn > 0 ? " (TRADE-IN " + std::to_string(tradeIn) +
                                     ")"
                               : "");
    SaveRacerProfile(profile, RacerProfilePath());
    return true;
}

}  // namespace sdl3cpp::services::impl
