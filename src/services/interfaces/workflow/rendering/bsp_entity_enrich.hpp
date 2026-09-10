#pragma once

#include "services/interfaces/workflow/rendering/bsp_types.hpp"

#include <map>
#include <nlohmann/json.hpp>
#include <string>
#include <unordered_map>
#include <array>
#include <vector>

namespace sdl3cpp::services::impl {

/// Fills in `id`, `position`, `model_index`/`bounds` and `kind`; records the
/// entity's world position (for later target resolution) and, for the first
/// info_player_deathmatch found, the spawn point.
void EnrichEntity(
    nlohmann::json& ent, const std::map<std::string, std::string>& values,
    const std::vector<BspModel>& models, float scale, int classIndex,
    std::unordered_map<std::string, std::array<float, 3>>& targets,
    nlohmann::json& spawn, int& pickupCount, int& jumpPadCount,
    int& teleporterCount);

}  // namespace sdl3cpp::services::impl
