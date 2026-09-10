#include "services/interfaces/workflow/rendering/bsp_texture_group_flattener.hpp"

namespace sdl3cpp::services::impl {

FlattenedBspGeometry FlattenBspTextureGroups(
    const std::map<int, TextureGroup>& groups, const std::string& mapName,
    int numTextures, const BspTexture* textures) {
    FlattenedBspGeometry out;
    out.vertices     = std::make_shared<std::vector<BspRenderVertex>>();
    out.indices      = std::make_shared<std::vector<uint32_t>>();
    out.usedTextures = std::make_shared<std::set<int>>();

    for (const auto& [texIdx, group] : groups) {
        if (group.indices.empty()) {
            continue;
        }

        const uint32_t vertexOffset =
            static_cast<uint32_t>(out.vertices->size());
        const uint32_t indexOffset = static_cast<uint32_t>(out.indices->size());

        out.vertices->insert(out.vertices->end(), group.vertices.begin(),
                             group.vertices.end());
        for (uint32_t idx : group.indices) {
            out.indices->push_back(idx + vertexOffset);
        }
        out.usedTextures->insert(texIdx);

        const bool hasName = texIdx >= 0 && texIdx < numTextures && textures;
        out.mapNodes.push_back(
            {{"name", "bsp_" + mapName},
             {"texture_index", texIdx},
             {"texture_name",
              hasName ? std::string(textures[texIdx].name) : std::string{}},
             {"index_offset", indexOffset},
             {"index_count", group.indices.size()}});
    }

    return out;
}

}  // namespace sdl3cpp::services::impl
