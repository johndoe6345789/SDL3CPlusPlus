#pragma once

#include "services/interfaces/workflow/gta5/vehicle/gta5_vehicle.hpp"

#include <cstdint>

namespace sdl3cpp::services::impl {

/// A car the game drives: a raycast vehicle like the player's own, so
/// it rides on its suspension, rolls its wheels and collides with
/// whatever it meets. The AI only works the pedals and the wheel.
struct Gta5TrafficCar {
    Gta5Vehicle car;
    std::uint32_t from{0};   // the node it left
    std::uint32_t to{0};     // the node it is making for
    float want{9.f};         // what it would do with a clear road
    float lane{2.2f};        // metres right of the centre line
    float stuck{0.f};        // seconds it has been going nowhere
};

}  // namespace sdl3cpp::services::impl
