#pragma once

namespace sdl3cpp::services::impl {

/// Handling figures for one pod, in engine units (about metres) and
/// seconds. Defaults approximate a stock pod: top speed ~145 m/s
/// (~520 km/h, the game's own speedometer range), boost to ~200.
struct RacerPodSpec {
    float topSpeed = 145.f;
    float boostSpeed = 200.f;
    float acceleration = 38.f;
    float brakeDeceleration = 90.f;
    float drag = 0.35f;            ///< speed lost per second when coasting
    float turnRate = 2.6f;         ///< radians per second at low speed
    float turnAtTopSpeed = 0.6f;   ///< share of it kept at top speed
    float hoverHeight = 1.4f;
    float gravity = 30.f;
    float stepHeight = 2.5f;       ///< higher ground ahead is a wall
    float heatRate = 0.28f;        ///< heat per second while boosting
    float coolRate = 0.18f;
    float repairRate = 0.25f;      ///< damage repaired per second
};

}  // namespace sdl3cpp::services::impl
