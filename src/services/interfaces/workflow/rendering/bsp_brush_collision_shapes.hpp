#pragma once

#include "services/interfaces/workflow/rendering/bsp_types.hpp"

#include <btBulletDynamicsCommon.h>

#include <cstdint>
#include <vector>

namespace sdl3cpp::services::impl {

/// Two compound shapes built from a BSP's brush lump: one for brushes that
/// block everything, one (possibly null) for player-clip-only brushes.
struct BspBrushCollisionShapes {
    btCompoundShape* solid = nullptr;
    btCompoundShape* clip  = nullptr;
    int solidBrushes       = 0;
    int clipBrushes        = 0;
    int skippedBrushes     = 0;
};

/**
 * @brief Builds solid/player-clip compound shapes from a BSP's brush lump.
 *
 * Skips brushes whose shader has neither CONTENTS_SOLID nor
 * CONTENTS_PLAYERCLIP, is SURF_NODRAW (except player-clip, which is always
 * nodraw), or whose convex hull has fewer than 4 vertices. The returned
 * `clip` shape is null when no player-clip brush was found; the caller owns
 * both shapes (and, transitively, their child btConvexHullShapes).
 */
BspBrushCollisionShapes BuildBspBrushCollisionShapes(
    const std::vector<uint8_t>& bspData, float scale);

}  // namespace sdl3cpp::services::impl
