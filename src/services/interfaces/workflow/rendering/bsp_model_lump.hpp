#pragma once

#include "services/interfaces/workflow/rendering/bsp_types.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace sdl3cpp::services::impl {

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
