#pragma once

#include "services/interfaces/workflow/gta5/vehicle/gta5_vehicle.hpp"

#include <string>

namespace sdl3cpp::services::impl {

/// Sets the suspension damping on every wheel. Stiff springs with light
/// damping bounce the wheels off every bump in the ground.
void DampSwitchbackWheels(Gta5Vehicle& car);

/// The current suspension length under each wheel, in metres, for the trace.
std::string DescribeSwitchbackSuspension(const Gta5Vehicle& car);

}  // namespace sdl3cpp::services::impl
