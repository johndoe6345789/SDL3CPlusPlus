#pragma once

#include <glm/glm.hpp>
#include <nlohmann/json.hpp>

namespace sdl3cpp::services::impl {

/// True and `out` set to the first trigger_teleport entity's
/// `target_position` found in `entities`; false if none has one.
bool FindPortalDestination(const nlohmann::json& entities, glm::vec3& out);

}  // namespace sdl3cpp::services::impl
