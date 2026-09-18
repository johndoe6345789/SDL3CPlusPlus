#include "services/interfaces/workflow/fs2024/data/gltf/fs2024_gltf_primitive.hpp"

#include "services/interfaces/workflow/fs2024/data/gltf/fs2024_gltf_accessor.hpp"

#include <algorithm>

namespace sdl3cpp::fs2024 {

using nlohmann::json;

std::vector<std::uint32_t> ReadGltfPrimitiveIndices(const json& gltf,
                                                    const std::uint8_t* bin,
                                                    const json& prim) {
    if (!prim.contains("indices")) return {};
    auto all = ReadIndexAccessor(gltf, bin, prim.at("indices").get<int>());
    const auto& asobo = prim.value("extras", json::object())
                            .value("ASOBO_primitive", json::object());
    if (!asobo.contains("PrimitiveCount")) return all;
    const auto start =
        std::min(asobo.value("StartIndex", std::size_t{0}), all.size());
    const auto end = std::min(
        start + asobo.at("PrimitiveCount").get<std::size_t>() * 3, all.size());
    std::vector<std::uint32_t> slice(all.begin() + start, all.begin() + end);
    const int baseVertex = asobo.value("BaseVertexIndex", 0);
    for (auto& index : slice) index += static_cast<std::uint32_t>(baseVertex);
    return slice;
}

std::string GltfBaseColorImageUri(const json& gltf, const json& prim) {
    if (!prim.contains("material")) return {};
    const auto& mat = gltf.at("materials").at(prim.at("material").get<int>());
    if (!mat.contains("pbrMetallicRoughness")) return {};
    const auto& pbr = mat.at("pbrMetallicRoughness");
    if (!pbr.contains("baseColorTexture")) return {};
    const int texIndex = pbr.at("baseColorTexture").at("index").get<int>();
    const auto& tex = gltf.at("textures").at(texIndex);
    const char* kDds = "MSFT_texture_dds";
    const bool dds = tex.contains("extensions") &&
                     tex.at("extensions").contains(kDds);
    const int imgIndex =
        dds ? tex.at("extensions").at(kDds).at("source").get<int>()
            : tex.value("source", -1);
    if (imgIndex < 0) return {};
    return gltf.at("images").at(imgIndex).value("uri", "");
}

}  // namespace sdl3cpp::fs2024
