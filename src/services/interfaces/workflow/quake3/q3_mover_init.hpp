#pragma once

#include "services/interfaces/workflow/quake3/q3_mover_types.hpp"

#include <nlohmann/json.hpp>

namespace sdl3cpp::services::impl {

/**
 * @brief Builds a Q3Mover for each func_door/func_plat entity.
 *
 * Reads each entity's "origin" (space-separated Q3 units, scaled by
 * 0.03125 to world units, same factor bsp.load uses), "angle" (degrees,
 * Q3 convention: 0=+X, 90=-Z in the XZ plane), "speed", "distance", and
 * "wait", deriving pos1/pos2/travelTime/waitTime.
 *
 * @param entities The parsed bsp.entities array; entities other than
 *                 func_door/func_plat are skipped. Returns an empty list
 *                 if `entities` is not an array.
 */
sdl3cpp::q3::MoverList BuildQ3MoversFromEntities(
    const nlohmann::json& entities);

}  // namespace sdl3cpp::services::impl
