#pragma once

#include "services/interfaces/workflow/rendering/bsp_model_lump.hpp"

#include <map>
#include <string>
#include <vector>

namespace sdl3cpp::services::impl {

/**
 * @brief Parses a Q3 BSP entity lump into per-entity key/value maps.
 *
 * The lump is a sequence of `{ "key" "value" ... }` blocks (Quake's own text
 * format, not JSON) — one block per entity. A block that fails to parse a
 * key/value pair is truncated there rather than aborting the whole lump, and
 * an empty block contributes nothing.
 */
std::vector<std::map<std::string, std::string>> ParseBspEntityLump(
    const std::string& entities);

}  // namespace sdl3cpp::services::impl
