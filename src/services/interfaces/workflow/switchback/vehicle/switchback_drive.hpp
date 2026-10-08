#pragma once

#include "services/interfaces/workflow/gta5/vehicle/gta5_vehicle.hpp"
#include "services/interfaces/workflow_context.hpp"

namespace sdl3cpp::services::impl {

/// The throttle the driver asks for, from the keys or the pad, in [0, 1].
float DriverThrottle(WorkflowContext& context);

/// Drives all four wheels with the rear push that gta5 sets, scaled by the
/// gearbox's power scale and by traction control. Returns the traction scale.
float ApplyFourWheelDrive(Gta5Vehicle& car, float powerScale,
                          float forwardSpeed);

}  // namespace sdl3cpp::services::impl
