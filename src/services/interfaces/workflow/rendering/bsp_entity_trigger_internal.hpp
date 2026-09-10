#pragma once

/// Internal helpers shared by bsp_entity_trigger_geometry.cpp and
/// bsp_entity_trigger.cpp, which together implement TryActivateTrigger()
/// from bsp_entity_update_helpers.hpp. Not part of the public
/// workflow-step API.

#include "services/interfaces/workflow/rendering/bsp_entity_update_helpers.hpp"

namespace sdl3cpp::services::impl::bsp_entity_trigger_detail {

bool AabbIntersectsBounds(const btVector3& bodyMin, const btVector3& bodyMax,
                          const nlohmann::json& bounds, float pad);

void TeleportBody(btRigidBody* body, const btVector3& dest);

btVector3 JumpPadVelocity(const btVector3& from, const btVector3& target);

}  // namespace sdl3cpp::services::impl::bsp_entity_trigger_detail
