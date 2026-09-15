#pragma once

#include <glm/glm.hpp>
#include <nlohmann/json.hpp>

#include <string>

namespace sdl3cpp::services::impl {

/// Reads a BSP entity's numeric field, which may be stored as either a
/// JSON number or (as BSP entity dictionaries always are) a string.
float EntFloat(const nlohmann::json& ent, const char* key, float def);

/// Parses a Q3 entity "x y z" origin string into world units, applying the
/// Quake-units-to-meters scale (default 1/32).
glm::vec3 ParseOrigin(const std::string& s, float scale = 0.03125f);

}  // namespace sdl3cpp::services::impl
