#pragma once

#include "services/interfaces/workflow/gta5/gta5_geometry.hpp"

#include <BulletDynamics/Vehicle/btRaycastVehicle.h>

namespace sdl3cpp::services::impl {

/// A car: what is drawn, plus the Bullet machinery driving it.
///
/// btRaycastVehicle is not a rigid body. It is an action attached to the
/// chassis body that casts a ray per wheel each step and applies
/// suspension and tyre forces, which is what gives a car springs and
/// grip instead of a box that slides.
struct Gta5Vehicle {
    Gta5Instance instance;
    btRigidBody* chassis{nullptr};
    btCollisionShape* chassisShape{nullptr};
    btVehicleRaycaster* raycaster{nullptr};
    btRaycastVehicle* vehicle{nullptr};
};

/// Wheel layout, in metres, derived from the chassis half extents.
struct Gta5WheelSetup {
    float radius{0.35f};
    float width{0.3f};
    float suspensionRest{0.35f};
    float connectionHeight{0.1f};
};

}  // namespace sdl3cpp::services::impl
