#pragma once

#include "services/interfaces/workflow/gta5/stream/gta5_geometry.hpp"
#include "services/interfaces/workflow/gta5/resource/gta5_placement.hpp"

namespace sdl3cpp::services::impl {

/// Put one instance into the physics world as static geometry, so the
/// q3.pm.* traces have something to stand on and slide along.
///
/// Instances at unit scale share the archetype's collision shape. Only a
/// scaled one needs a wrapper of its own, which the instance then owns.
bool AddGta5InstanceBody(btDiscreteDynamicsWorld* world,
                         const Gta5Placement& placement,
                         Gta5Geometry& geometry, Gta5Instance& instance);

/// Take the instance's body back out of the world and delete it.
void RemoveGta5InstanceBody(btDiscreteDynamicsWorld* world,
                            Gta5Instance& instance);

}  // namespace sdl3cpp::services::impl
