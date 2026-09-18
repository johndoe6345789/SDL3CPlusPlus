#include "services/interfaces/workflow/fs2024/data/gltf/fs2024_gltf_model.hpp"

#include "services/interfaces/workflow/fs2024/data/bgl/fs2024_riff_chunk.hpp"
#include "services/interfaces/workflow/fs2024/data/gltf/fs2024_gltf_glb.hpp"
#include "services/interfaces/workflow/fs2024/data/gltf/fs2024_gltf_gxml.hpp"

#include <cstring>
#include <stdexcept>

namespace sdl3cpp::fs2024 {
namespace {

/// A model blob's LOD list and one binary glTF per LOD, located without
/// decoding any of them.
struct ModelParts {
    std::vector<GltfLodHeader> headers;
    std::vector<RiffChunk> glbs;
};

ModelParts SplitModel(const std::vector<std::uint8_t>& riff) {
    if (riff.size() < 12 || std::memcmp(riff.data(), "RIFF", 4) != 0) {
        throw std::runtime_error("model RIFF: bad magic");
    }
    ModelParts parts;
    for (const auto& chunk :
         WalkRiffChunks(riff.data() + 12, riff.size() - 12)) {
        if (chunk.id == "GXML") {
            parts.headers = ParseGxmlLods(std::string(
                reinterpret_cast<const char*>(chunk.data), chunk.size));
        } else if (chunk.id == "GLBD") {
            parts.glbs = WalkRiffChunks(chunk.data, chunk.size);
        }
    }
    return parts;
}

/// `lod` named as the GXML list names LOD `index`.
template <typename Lod>
Lod Named(const ModelParts& parts, std::size_t index, Lod lod) {
    if (index < parts.headers.size()) {
        lod.modelFile = parts.headers[index].modelFile;
        lod.minSize = parts.headers[index].minSize;
    }
    return lod;
}

GltfLod Decode(const ModelParts& parts, std::size_t index) {
    const RiffChunk& glb = parts.glbs.at(index);
    return Named(parts, index, ParseLodGlb(glb.data, glb.size));
}

}  // namespace

std::vector<GltfLod> ParseModelRiff(const std::vector<std::uint8_t>& riff) {
    const ModelParts parts = SplitModel(riff);
    std::vector<GltfLod> lods;
    for (std::size_t i = 0; i < parts.glbs.size(); ++i) {
        lods.push_back(Decode(parts, i));
    }
    return lods;
}

std::vector<GltfLodInfo> ListModelRiffLods(
    const std::vector<std::uint8_t>& riff) {
    const ModelParts parts = SplitModel(riff);
    std::vector<GltfLodInfo> lods;
    for (std::size_t i = 0; i < parts.glbs.size(); ++i) {
        GltfLodInfo info;
        info.bytes = parts.glbs[i].size;
        lods.push_back(Named(parts, i, info));
    }
    return lods;
}

GltfLod ParseModelRiffLod(const std::vector<std::uint8_t>& riff,
                          std::size_t index) {
    return Decode(SplitModel(riff), index);  // throws when out of range
}

}  // namespace sdl3cpp::fs2024
