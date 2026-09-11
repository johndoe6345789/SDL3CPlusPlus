#pragma once

#include "services/interfaces/workflow/gta5/gta5_geometry.hpp"

#include <glm/glm.hpp>

namespace sdl3cpp::services::impl {

/// Build the dynamic body for a vehicle, sized to its collision mesh.
///
/// The shape is a box rather than the render mesh: a concave car shell
/// makes a poor dynamic collider and Bullet will not solve it against
/// the ground. `outShape` receives the box, which the caller owns.
btRigidBody* MakeGta5VehicleBody(const Gta5Geometry& geometry,
                                 const glm::vec3& position, float mass,
                                 btCollisionShape*& outShape);

}  // namespace sdl3cpp::services::impl
