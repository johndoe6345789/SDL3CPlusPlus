#pragma once

#include "services/interfaces/workflow/rendering/bsp_types.hpp"

#include <map>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

namespace sdl3cpp::services::impl {

/// Everything bsp.parse_spawn publishes, built from a lump's parsed entities.
struct BspEntitiesResult {
    nlohmann::json entities = nlohmann::json::array();
    nlohmann::json spawn;
    int pickupCount     = 0;
    int jumpPadCount    = 0;
    int teleporterCount = 0;
};

/**
 * @brief Converts parsed Quake entities into the engine's entity JSON.
 *
 * Converts each entity's `origin` from Q3 Z-up to engine Y-up (scaled), picks
 * the deathmatch spawn point (first `info_player_deathmatch` found), resolves
 * each entity's `target` to the named entity's position, attaches brush-model
 * bounds for entities with a `model "*N"` reference, and classifies pickups /
 * jump pads / teleporters by classname.
 *
 * @param parsedEntities Key/value maps from ParseBspEntityLump().
 * @param models         Brush models from the BSP's LUMP_MODELS, indexed by
 *                        the `*N` suffix of an entity's `model` field.
 * @param scale          Q3 world unit -> engine unit scale.
 */
BspEntitiesResult BuildBspEntitiesJson(
    const std::vector<std::map<std::string, std::string>>& parsedEntities,
    const std::vector<BspModel>& models, float scale);

}  // namespace sdl3cpp::services::impl
