#pragma once

#include "services/interfaces/workflow/gta5/vehicle/gta5_vehicle.hpp"

namespace sdl3cpp::services::impl {

/// Brakes all four wheels and cuts the drive and the steering, so a car at
/// rest stays where it is while the menu is open or the race is finished.
void HoldSwitchbackCar(Gta5Vehicle& car);

}  // namespace sdl3cpp::services::impl
