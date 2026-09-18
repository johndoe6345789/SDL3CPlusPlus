#include "services/interfaces/workflow/fs2024/data/gltf/fs2024_gltf_glb.hpp"

#include "services/interfaces/workflow/fs2024/data/gltf/fs2024_gltf_primitive.hpp"
#include "services/interfaces/workflow/fs2024/data/gltf/fs2024_gltf_scene.hpp"

#include <nlohmann/json.hpp>

#include <cstring>
#include <string>

namespace sdl3cpp::fs2024 {
namespace {

struct GlbChunks {
    const std::uint8_t* json = nullptr;
    std::size_t jsonLen = 0;
    const std::uint8_t* bin = nullptr;
};

/// GLB's own chunk header is `length(u32) + type(4)`, the reverse field
/// order from the RIFF chunks wrapping it, so this does not reuse
/// `WalkRiffChunks`.
GlbChunks SplitGlb(const std::uint8_t* data, std::size_t size) {
    GlbChunks out;
    std::size_t pos = 12;
    while (pos + 8 <= size) {
        std::uint32_t length = 0;
        std::memcpy(&length, data + pos, 4);
        const std::uint8_t* payload = data + pos + 8;
        if (std::memcmp(data + pos + 4, "JSON", 4) == 0) {
            out.json = payload;
            out.jsonLen = length;
        } else if (std::memcmp(data + pos + 4, "BIN\0", 4) == 0) {
            out.bin = payload;
        }
        pos += 8 + length + (length & 1);
    }
    return out;
}

}  // namespace

GltfLod ParseLodGlb(const std::uint8_t* data, std::size_t size) {
    const auto split = SplitGlb(data, size);
    std::string text(reinterpret_cast<const char*>(split.json), split.jsonLen);
    while (!text.empty() && (text.back() == '\0' || text.back() == ' ')) {
        text.pop_back();
    }
    const nlohmann::json gltf = nlohmann::json::parse(text);
    const auto transforms = ComputeNodeWorldTransforms(gltf);

    GltfLod lod;
    const auto& nodes = gltf.at("nodes");
    for (std::size_t i = 0; i < nodes.size(); ++i) {
        if (!nodes[i].contains("mesh")) continue;
        const auto& mesh = gltf.at("meshes").at(nodes[i].at("mesh").get<int>());
        for (const auto& prim : mesh.at("primitives")) {
            lod.primitives.push_back(
                BuildGltfPrimitive(gltf, split.bin, prim, transforms[i]));
        }
    }
    return lod;
}

}  // namespace sdl3cpp::fs2024
