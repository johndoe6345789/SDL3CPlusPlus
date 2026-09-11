#pragma once

#include "services/interfaces/workflow/gta5/gta5_stream_state.hpp"

#include <btBulletDynamicsCommon.h>
#include <glm/glm.hpp>

namespace sdl3cpp::services::impl {

/// Freeze every car with nothing under it: pinned where it was, at rest,
/// until a downward ray finds ground again.
///
/// Read on demand, a district takes seconds to arrive, and a car driven
/// faster than the map streams would fall through the empty world. It
/// asks about the ground itself, not whether nearby tiles are complete:
/// a neighbour rebuilt for a LOD change froze the car on a road that was
/// there all along. Runs after the physics step, so a car moves at most
/// one frame before it is put back.
void HoldGta5VehiclesOverMissingGround(Gta5StreamState& state,
                                       btDiscreteDynamicsWorld* world);

/// The first surface straight below `from`, searched from 5 m above it
/// to 500 m below -- so the given height picks which level of a stacked
/// road is meant. False when there is nothing there.
bool Gta5GroundBelow(btDiscreteDynamicsWorld* world, const glm::vec3& from,
                     float& groundY);

}  // namespace sdl3cpp::services::impl
