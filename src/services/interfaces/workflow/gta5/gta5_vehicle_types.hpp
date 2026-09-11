#pragma once

#include "services/interfaces/workflow/gta5/gta5_geometry.hpp"

#include <array>

#include <LinearMath/btVector3.h>

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
    /// The four wheels as their own meshes, placed from the physics
    /// wheel transforms so they spin and steer. Empty when the model
    /// had no separable wheels.
    std::array<Gta5Instance, 4> wheels{};
    bool hasWheels{false};
    btRigidBody* chassis{nullptr};
    btCollisionShape* chassisShape{nullptr};
    btVehicleRaycaster* raycaster{nullptr};
    btRaycastVehicle* vehicle{nullptr};
};

/// Wheel layout in metres.
///
/// The axle positions come from the model rather than the chassis
/// bounding box: a box puts the wheels out at the bumpers, which leaves
/// the wheel meshes visibly detached from the car.
struct Gta5WheelSetup {
    float radius{0.37f};
    float width{0.3f};
    float suspensionRest{0.2f};
    float connectionHeight{0.1f};
    std::array<btVector3, 4> axles{};
    bool hasAxles{false};
};

}  // namespace sdl3cpp::services::impl
