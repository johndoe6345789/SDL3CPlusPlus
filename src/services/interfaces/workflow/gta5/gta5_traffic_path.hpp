#pragma once

#include "services/interfaces/workflow/gta5/gta5_effects.hpp"
#include "services/interfaces/workflow/gta5/gta5_traffic.hpp"

namespace sdl3cpp::services::impl {

/// The next road out of `node`, never straight back the way it came
/// unless that is the only way out -- a dead end has to be turned in.
std::uint32_t NextGta5Link(const Gta5Roads& roads, std::uint32_t node,
                           std::uint32_t from);

/// A roll of the traffic's own die, for picking among roads.
std::uint32_t Gta5TrafficRoll();

/// How far the nearest car ahead on the same road is, or a clear road.
float Gta5GapAhead(const Gta5Traffic& traffic, const Gta5TrafficCar& car);

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
