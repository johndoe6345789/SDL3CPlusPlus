#pragma once

#include "services/interfaces/workflow/gta5/gta5_stream_state.hpp"

#include <btBulletDynamicsCommon.h>
#include <glm/glm.hpp>

namespace sdl3cpp::services::impl {

/// Freeze every car whose ground has not streamed in: pinned where it
/// was when its tile went missing, at rest, until the tile is complete.
///
/// Read on demand, a district takes seconds to arrive. Without this a car
/// waiting at spawn, or driven faster than the map streams, falls through
/// the empty world. Runs after the physics step, so a car moves at most
/// one frame before it is put back.
void HoldGta5VehiclesOverMissingGround(Gta5StreamState& state);

/// The first surface straight below `from`, searched from 100 m above
/// it to 500 m below. False when there is nothing there.
bool Gta5GroundBelow(btDiscreteDynamicsWorld* world, const glm::vec3& from,
                     float& groundY);

}  // namespace sdl3cpp::services::impl
