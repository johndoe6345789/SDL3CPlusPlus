#pragma once

#include "services/interfaces/i_logger.hpp"
#include "services/interfaces/workflow/gta5/gta5_stream_state.hpp"
#include "services/interfaces/workflow_context.hpp"

#include <btBulletDynamicsCommon.h>

#include <memory>

namespace sdl3cpp::services::impl {

/// Check the player for a non-finite position or velocity, every frame.
///
/// Getting out of a fast car could turn the player's position to NaN,
/// and the camera, the streamer and everything drawn went with it. The
/// first time it happens this logs the player, the movement state and
/// every car, with the last finite values; every time, it puts the
/// player back where they last were, at rest.
void GuardGta5PlayerFinite(Gta5StreamState& state, WorkflowContext& context,
                           btRigidBody* player,
                           const std::shared_ptr<ILogger>& logger);

}  // namespace sdl3cpp::services::impl
