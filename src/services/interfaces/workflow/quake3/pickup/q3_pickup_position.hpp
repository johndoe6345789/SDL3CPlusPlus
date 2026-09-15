#pragma once

#include <glm/glm.hpp>
#include <nlohmann/json.hpp>

namespace sdl3cpp::services::impl {

/**
 * @brief Reads a BSP entity's world position.
 *
 * Accepts either an "origin" field holding an "x y z" string, or a
 * "position" field holding a 3-element JSON array.
 *
 * @return true with `out` set; false if neither field is present/parseable.
 */
bool ReadEntityPosition(const nlohmann::json& ent, glm::vec3& out);

}  // namespace sdl3cpp::services::impl
