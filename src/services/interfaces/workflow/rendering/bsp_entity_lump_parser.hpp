#pragma once

#include "services/interfaces/workflow/rendering/bsp_types.hpp"

#include <cstdint>
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

/// Parses "x y z" (Quake's `origin`/similar fields); false if malformed.
bool ParseBspVec3(const std::string& text, float& x, float& y, float& z);

/**
 * @brief Reads the BSP's brush models (LUMP_MODELS) into a typed vector.
 *
 * Entities reference these by index (a `model "*N"` field) to attach a brush
 * model's bounds; returns empty if the lump's offset/length don't fit inside
 * `bspData`.
 */
std::vector<BspModel> ReadBspModels(const std::vector<uint8_t>& bspData,
                                    const BspLump& modelLump);

}  // namespace sdl3cpp::services::impl
