#pragma once

#include "services/interfaces/workflow/gta5/effects/gta5_effects.hpp"
#include "services/interfaces/workflow/gta5/traffic/gta5_traffic.hpp"

namespace sdl3cpp::services::impl {

/// The next road out of `node`, never straight back the way it came
/// unless that is the only way out -- a dead end has to be turned in.
std::uint32_t NextGta5Link(const Gta5Roads& roads, std::uint32_t node,
                           std::uint32_t from);

/// A road within the ring around `at` to put a new car on, picked at
/// random among all of them so the traffic does not queue onto one.
bool Gta5TrafficSpot(const Gta5Roads& roads, const Gta5Traffic& traffic,
                     const glm::vec3& at, std::uint32_t& from,
                     std::uint32_t& to);

/// A roll of the traffic's own die, for picking among roads.
std::uint32_t Gta5TrafficRoll();

/// How fast a car should be going: what it would like, held down for
/// the car in front and for a light standing against it.
float Gta5TrafficWant(const Gta5Traffic& traffic, const Gta5Roads& roads,
                      const Gta5TrafficCar& car, const glm::vec3& at,
                      float facing);

/// Hang one crossing's lamps on the map's own traffic light props,
/// if any have streamed in near it. One crossing a frame, so a
/// district arriving costs nothing.
void FindGta5Lamps(const Gta5StreamState& state, Gta5Traffic& traffic,
                   const Gta5Roads& roads, float dt);

/// A lamp over every crossing in `traffic`, showing which way is
/// going: green, amber on the change, and red for the arm held.
void ShowGta5Lights(Gta5Effects& effects, const Gta5Traffic& traffic,
                    const Gta5Roads& roads);

}  // namespace sdl3cpp::services::impl
