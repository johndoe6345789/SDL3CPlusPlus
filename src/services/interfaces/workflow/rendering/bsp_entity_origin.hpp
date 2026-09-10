#pragma once

#include <array>
#include <map>
#include <nlohmann/json.hpp>
#include <string>
#include <unordered_map>

namespace sdl3cpp::services::impl {

/// Converts `origin`, records it under `targetname` for later target
/// resolution, and captures the first info_player_deathmatch as `spawn`.
void ApplyEntityOrigin(
    nlohmann::json& ent, const std::map<std::string, std::string>& values,
    const std::string& classname, float scale,
    std::unordered_map<std::string, std::array<float, 3>>& targets,
    nlohmann::json& spawn);

}  // namespace sdl3cpp::services::impl
