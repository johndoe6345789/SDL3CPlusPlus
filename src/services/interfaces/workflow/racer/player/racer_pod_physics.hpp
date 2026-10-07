#pragma once

#include "services/interfaces/workflow/racer/player/racer_pod_spec.hpp"
#include "services/interfaces/workflow/racer/player/racer_surface.hpp"

#include <glm/glm.hpp>

#include <array>
#include <optional>

namespace sdl3cpp::services::impl {

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
    /// 0..1 per engine, left then right. Damage limits top speed, and
    /// an engine weaker than the other pulls the pod toward its side.
    std::array<float, 2> engineDamage{0.f, 0.f};
    float overheatTimer = 0.f;
    bool boosting = false;
    bool grounded = false;
    bool blocked = false;   ///< against a wall it cannot slide along
    float airTime = 0.f;
    float stuckTime = 0.f;  ///< seconds blocked or crawling
    std::uint32_t surface = 0;  ///< RacerSurfaceFlag bits under the pod
    /// The pod's body for contact with other pods: metres ahead of and
    /// behind its origin, half its width, and its mass.
    float bodyFront = 7.f;
    float bodyBack = 19.f;
    float bodyHalfWidth = 2.2f;
    float mass = 50.f;
};

/// Boost, heat, overheating and repair for one tick.
void UpdateRacerPodEngines(RacerPodState& pod, const RacerPodInput& input,
                           const RacerPodSpec& spec, float dt);

/// The speed the engines are driving toward this tick.
float RacerPodTargetSpeed(const RacerPodState& pod,
                          const RacerPodInput& input,
                          const RacerPodSpec& spec);

/// The pod's overall damage: the mean of its engines'.
float RacerPodDamage(const RacerPodState& pod);

/// Forward unit vector for a heading.
glm::vec3 RacerPodForward(float heading);

/// Moves the pod forward over the surface, or failing that slides it
/// along the wall by trying the move turned progressively further
/// aside, losing speed. False (and half the speed) when every way is
/// blocked. `next` receives the new horizontal position.
bool MoveRacerPodAlongSurface(RacerPodState& pod, const RacerPodSpec& spec,
                              float dt, const RacerSurface& surface,
                              glm::vec3& next);

/// Advances one pod by `dt` seconds against the track surface.
void StepRacerPod(RacerPodState& pod, const RacerPodInput& input,
                  const RacerPodSpec& spec, float dt,
                  const RacerSurface& surface);

}  // namespace sdl3cpp::services::impl
