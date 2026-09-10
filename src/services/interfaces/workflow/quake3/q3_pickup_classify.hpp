#pragma once

#include <glm/glm.hpp>
#include <nlohmann/json.hpp>
#include <string>

namespace sdl3cpp::services::impl {

/// True if `value` starts with `prefix`.
bool HasPrefix(const std::string& value, const std::string& prefix);

/// True if `classname` names a Quake3 pickup entity (weapon, ammo, item,
/// or holdable).
bool IsPickup(const std::string& classname);

/// Maps a pickup classname to the texture-cache key used to render it.
std::string TextureKeyForClass(const std::string& classname);

/// Parses `value` as a 3-element JSON array into `out`. Returns false
/// (leaving `out` unchanged) if `value` isn't a 3-element array.
bool ReadVec3(const nlohmann::json& value, glm::vec3& out);

}  // namespace sdl3cpp::services::impl
