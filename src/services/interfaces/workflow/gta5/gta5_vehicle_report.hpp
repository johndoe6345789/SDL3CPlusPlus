#pragma once

#include "services/interfaces/workflow/gta5/gta5_vehicle_types.hpp"

#include <string>

namespace sdl3cpp::services::impl {

/// One line on what a car is doing: position, speed, whether it is held,
/// and per wheel whether it touches the ground, its suspension length and
/// the engine and suspension forces on it.
///
/// A car that will not move is held, or off its wheels, or resting on its
/// chassis box, or getting no engine force -- and those look identical
/// from outside.
std::string DescribeGta5Vehicle(const Gta5Vehicle& car);

}  // namespace sdl3cpp::services::impl
