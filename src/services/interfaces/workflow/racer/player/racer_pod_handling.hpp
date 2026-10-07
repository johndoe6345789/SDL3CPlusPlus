#pragma once

#include "services/interfaces/workflow/racer/player/racer_pod_spec.hpp"

#include <array>

namespace sdl3cpp::services::impl {

/// The game's pod handling record, field by field (racer_tracks.json
/// names them in handling_fields).
enum class RacerHandling {
    AntiSkid, TurnResponse, MaxTurnRate, Acceleration, MaxSpeed,
    AirBrakeInv, DecelInv, BoostThrust, HeatRate, CoolRate, HoverHeight,
    RepairRate, BumpMass, DamageImmunity, ContactRadius, Count
};

using RacerHandlingRecord =
    std::array<float, static_cast<int>(RacerHandling::Count)>;

/// A pod spec from the game's figures: speeds read as km/h, turn rates
/// as degrees per second, heat and cool rates against Anakin's (13, 9).
/// Pure, so it is testable.
RacerPodSpec RacerSpecFromHandling(const RacerHandlingRecord& record);

}  // namespace sdl3cpp::services::impl
