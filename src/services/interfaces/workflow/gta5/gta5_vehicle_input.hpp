#pragma once

#include "services/interfaces/workflow_context.hpp"

#include <btBulletDynamicsCommon.h>
#include <nlohmann/json.hpp>

namespace sdl3cpp::services::impl {

/// input.keyboard.poll records only the keys that are down, by SDL
/// scancode name ("F", "W", "Space").
bool Gta5KeyDown(const nlohmann::json* keys, const char* name);

/// The player's rigid body, named by physics_player_body.
btRigidBody* Gta5PlayerBody(WorkflowContext& context);

}  // namespace sdl3cpp::services::impl
