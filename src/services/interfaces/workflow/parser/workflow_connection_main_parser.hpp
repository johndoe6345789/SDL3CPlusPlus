#pragma once

#include <rapidjson/document.h>

#include <string>
#include <utility>
#include <vector>

namespace sdl3cpp::services::impl {

/**
 * @brief Appends `fromNode`'s edges from its connections["main"] value.
 *
 * Supports both n8n's object-of-branches format
 * (`"main": { "0": [...], "1": [...] }`) and the simple array-of-branches
 * format (`"main": [[...]]`).
 *
 * @throws std::runtime_error if `mainValue` or any branch/entry is
 *         malformed.
 */
void ParseMainConnections(
    const rapidjson::Value& mainValue, const std::string& fromNode,
    std::vector<std::pair<std::string, std::string>>& edges);

}  // namespace sdl3cpp::services::impl
