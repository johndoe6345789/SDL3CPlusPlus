#pragma once

#include "services/interfaces/workflow/bl4/bl4_geometry.hpp"
#include "services/interfaces/workflow/bl4/bl4_placement.hpp"

namespace sdl3cpp::services::impl {

/// Puts one instance into the physics world as static geometry, so the
/// q3.pm.* traces have something to stand on and slide along.
///
/// Instances at unit scale share the archetype's collision shape. Only a
/// scaled one needs a wrapper of its own, which the instance then owns.
bool AddBl4InstanceBody(btDiscreteDynamicsWorld* world, const Bl4Placement& placement,
                        Bl4Geometry& geometry, Bl4Instance& instance);

/// Takes the instance's body back out of the world and deletes it.
void RemoveBl4InstanceBody(btDiscreteDynamicsWorld* world, Bl4Instance& instance);

}  // namespace sdl3cpp::services::impl
