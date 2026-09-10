#pragma once

#include "services/interfaces/workflow/rendering/bsp_types.hpp"

#include <map>
#include <memory>
#include <nlohmann/json.hpp>
#include <set>
#include <string>
#include <vector>

namespace sdl3cpp::services::impl {

/// Result of flattening a per-texture group map into one draw list.
struct FlattenedBspGeometry {
    std::shared_ptr<std::vector<BspRenderVertex>> vertices;
    std::shared_ptr<std::vector<uint32_t>> indices;
    std::shared_ptr<std::set<int>> usedTextures;
    nlohmann::json mapNodes = nlohmann::json::array();
};

/**
 * @brief Concatenates every non-empty texture group into one VB/IB.
 *
 * Each group becomes one `map.nodes` entry naming its index range, so a later
 * draw step can bind one buffer pair and issue one indexed draw call per
 * group instead of one per BSP face.
 *
 * @param numTextures/textures Used to resolve each group's texture name;
 *                              pass numTextures=0 to leave names blank.
 */
FlattenedBspGeometry FlattenBspTextureGroups(
    const std::map<int, TextureGroup>& groups, const std::string& mapName,
    int numTextures, const BspTexture* textures);

}  // namespace sdl3cpp::services::impl
