#pragma once

#include "services/interfaces/workflow/gta5/vehicle/gta5_vehicle_types.hpp"

#include <BulletDynamics/Dynamics/btDiscreteDynamicsWorld.h>

namespace sdl3cpp::services::impl {

/// Attach a raycast vehicle with four sprung wheels to a chassis body.
///
/// Fills `out.raycaster` and `out.vehicle`, both owned by the vehicle.
void AttachGta5Wheels(Gta5Vehicle& out, btDiscreteDynamicsWorld* world,
                      const btVector3& halfExtents,
                      const Gta5WheelSetup& setup);

}  // namespace sdl3cpp::services::impl
