#pragma once

#include <glm/glm.hpp>

#include <optional>

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
    float turnRate = 1.9f;         ///< radians per second at low speed
    float hoverHeight = 1.4f;
    float gravity = 30.f;
    float stepHeight = 2.5f;       ///< higher ground ahead is a wall
    float heatRate = 0.28f;        ///< heat per second while boosting
    float coolRate = 0.18f;
    float repairRate = 0.25f;      ///< damage repaired per second
};

struct RacerPodInput {
    float throttle = 0.f;   ///< -1 (brake/reverse) .. 1
    float steer = 0.f;      ///< -1 left .. 1 right
    bool boost = false;
    bool repair = false;
};

struct RacerPodState {
    glm::vec3 position{0.f};
    float heading = 0.f;    ///< radians; forward = (sin h, 0, -cos h)
    float speed = 0.f;
    float verticalSpeed = 0.f;
    float heat = 0.f;       ///< 0..1; at 1 an engine overheats
    float damage = 0.f;     ///< 0..1; limits top speed
    float overheatTimer = 0.f;
    bool boosting = false;
    bool grounded = false;
    float airTime = 0.f;
};

/// Boost, heat, overheating and repair for one tick.
void UpdateRacerPodEngines(RacerPodState& pod, const RacerPodInput& input,
                           const RacerPodSpec& spec, float dt);

/// The speed the engines are driving toward this tick.
float RacerPodTargetSpeed(const RacerPodState& pod,
                          const RacerPodInput& input,
                          const RacerPodSpec& spec);

/// Forward unit vector for a heading.
glm::vec3 RacerPodForward(float heading);

/// Surface height under a point, or nothing (a gap).
using RacerGroundProbe = std::optional<float> (*)(const void* ground,
                                                  float x, float z,
                                                  float ceiling);

/// Advances one pod by `dt` seconds against the track surface.
void StepRacerPod(RacerPodState& pod, const RacerPodInput& input,
                  const RacerPodSpec& spec, float dt,
                  RacerGroundProbe probe, const void* ground);

}  // namespace sdl3cpp::services::impl
