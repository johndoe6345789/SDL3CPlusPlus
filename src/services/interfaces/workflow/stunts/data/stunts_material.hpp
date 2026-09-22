#pragma once

#include <cstdint>
#include <map>
#include <string>

#include <glm/glm.hpp>

namespace sdl3cpp::services::impl {

/// Stunts' shape materials, keyed by id, as decoded RGB colours.
struct StuntsMaterialTable {
    std::map<std::uint8_t, glm::vec3> colors;
    glm::vec3 fallback{0.565f};  // #909090: an honest "unknown" grey.
    bool loaded = false;
};

/// Reads the JSON asset (`packages/stunts/assets/stunts_materials.json`).
StuntsMaterialTable LoadStuntsMaterialTable(const std::string& path);

/// `id`'s colour, or the table's fallback if it is not in the table.
glm::vec3 StuntsColorFor(const StuntsMaterialTable& table, std::uint8_t id);

}  // namespace sdl3cpp::services::impl
