#pragma once

namespace sdl3cpp::services::impl {

/// Handling figures for one pod, in metres and seconds. The defaults are
/// Anakin's stock pod; RacerSpecFromHandling fills them in for any racer
/// from the game's own table (top speed 490 there reads as 490 km/h).
struct RacerPodSpec {
    float topSpeed = 136.f;        ///< m/s (490 km/h)
    float boostSpeed = 192.f;      ///< m/s with the boost lit
    float acceleration = 30.f;
    float brakeDeceleration = 90.f;
    float drag = 0.35f;            ///< speed lost per second when coasting
    float turnRate = 2.5f;         ///< radians per second at low speed
    float turnAtTopSpeed = 0.6f;   ///< share of it kept at top speed
    float antiSkid = 0.5f;         ///< grip kept on slippery ground, 0..1
    float hoverHeight = 1.5f;
    float gravity = 20.f;          ///< pods float a little over jumps
    float stepHeight = 2.5f;       ///< higher ground ahead is a wall
    float heatRate = 0.28f;        ///< heat per second while boosting
    float coolRate = 0.18f;
    float repairRate = 0.25f;      ///< damage repaired per second
    float bumpMass = 50.f;         ///< shares the push when pods touch
    float damageImmunity = 0.6f;   ///< 0 takes all damage, 1 none
};

}  // namespace sdl3cpp::services::impl
