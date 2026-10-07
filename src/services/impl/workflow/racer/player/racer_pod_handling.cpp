#include "services/interfaces/workflow/racer/player/racer_pod_handling.hpp"

#include <algorithm>

namespace sdl3cpp::services::impl {
namespace {

constexpr float kKmhToMs = 1.f / 3.6f;
constexpr float kDegrees = 3.14159265f / 180.f;

}  // namespace

RacerPodSpec RacerSpecFromHandling(const RacerHandlingRecord& record) {
    auto at = [&record](RacerHandling field) {
        return record[static_cast<int>(field)];
    };
    RacerPodSpec spec;
    if (at(RacerHandling::MaxSpeed) <= 0.f) return spec;  // no record
    spec.topSpeed = at(RacerHandling::MaxSpeed) * kKmhToMs;
    spec.boostSpeed = (at(RacerHandling::MaxSpeed) +
                       at(RacerHandling::BoostThrust)) * kKmhToMs;
    // The game's acceleration figure behaves like a time: Ben
    // Quadinaros' four engines (1.0) pull hardest, Gasgano (3.75) least.
    spec.acceleration =
        spec.topSpeed / (1.2f + 1.2f * at(RacerHandling::Acceleration));
    // Inverse figures: lower is a stronger air brake and engine drag.
    spec.brakeDeceleration = 90.f * 30.f /
                             std::max(10.f, at(RacerHandling::AirBrakeInv));
    spec.drag = 0.35f * 60.f / std::max(20.f, at(RacerHandling::DecelInv));
    spec.turnRate = 1.3f * kDegrees * at(RacerHandling::MaxTurnRate);
    spec.turnAtTopSpeed = std::clamp(
        0.6f * at(RacerHandling::TurnResponse) / 300.f, 0.3f, 0.9f);
    spec.antiSkid = std::clamp(at(RacerHandling::AntiSkid), 0.f, 1.f);
    spec.heatRate = 0.28f * at(RacerHandling::HeatRate) / 13.f;
    spec.coolRate = std::max(0.05f, 0.18f * at(RacerHandling::CoolRate) /
                                        9.f);
    spec.repairRate = 0.625f * at(RacerHandling::RepairRate);
    spec.bumpMass = at(RacerHandling::BumpMass);
    spec.damageImmunity =
        std::clamp(at(RacerHandling::DamageImmunity), 0.f, 0.95f);
    return spec;
}

}  // namespace sdl3cpp::services::impl
