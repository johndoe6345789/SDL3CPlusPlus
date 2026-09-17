#include "services/interfaces/workflow/fs2024/prepare/fs2024_gltf_model.hpp"

#include "services/interfaces/workflow/fs2024/prepare/fs2024_gltf_accessor.hpp"
#include "services/interfaces/workflow/fs2024/prepare/fs2024_gltf_scene.hpp"
#include "services/interfaces/workflow/fs2024/prepare/fs2024_riff_chunk.hpp"

#include <algorithm>
#include <cctype>
#include <cstddef>
#include <cstring>
#include <glm/mat3x3.hpp>
#include <glm/vec4.hpp>
#include <nlohmann/json.hpp>
#include <stdexcept>

namespace sdl3cpp::tools::fs2024 {
namespace {
using nlohmann::json;
using services::impl::BspRenderVertex;

struct LodHeader {
    std::string modelFile;
    float minSize = 0.f;
};

struct GlbChunks {
    const std::uint8_t* json = nullptr;
    std::size_t jsonLen = 0;
    const std::uint8_t* bin = nullptr;
    std::size_t binLen = 0;
};

/// `name="value"` from `tag`, matched case-insensitively (FS2024's
/// own GXML mixes `MinSize=`/`minSize=` across models).
std::string AttrValueCi(const std::string& tag, const std::string& name) {
    std::string lowerTag = tag, needle = name + "=\"";
    for (char& c : lowerTag) c = static_cast<char>(std::tolower(c));
    for (char& c : needle) c = static_cast<char>(std::tolower(c));
    const auto at = lowerTag.find(needle);
    if (at == std::string::npos) return {};
    const auto start = at + needle.size();
    const auto end = tag.find('"', start);
    return end == std::string::npos ? std::string()
                                    : tag.substr(start, end - start);
}

std::vector<LodHeader> ParseGxmlLods(const std::string& xml) {
    std::vector<LodHeader> lods;
    std::size_t pos = 0;
    // The trailing space excludes the wrapping "<LODS>" element, which
    // "<LOD" alone would also match.
    while ((pos = xml.find("<LOD ", pos)) != std::string::npos) {
        const auto end = xml.find('>', pos);
        if (end == std::string::npos) break;
        const std::string tag = xml.substr(pos, end - pos);
        LodHeader lod;
        lod.modelFile = AttrValueCi(tag, "ModelFile");
        const auto sizeStr = AttrValueCi(tag, "minsize");
        lod.minSize = sizeStr.empty() ? 0.f : std::stof(sizeStr);
        lods.push_back(std::move(lod));
        pos = end;
    }
    return lods;
}

/// Splits a binary glTF (`glTF`+version+length, then a JSON chunk and
/// a BIN chunk) -- GLB's own chunk header is `length(u32)+type(4)`,
/// the reverse field order from the RIFF chunks wrapping it, so this
/// does not reuse `WalkRiffChunks`.
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
            out.binLen = length;
        }
        pos += 8 + length + (length & 1);
    }
    return out;
}

GltfPrimitive BuildPrimitive(const json& gltf, const std::uint8_t* bin,
                            const json& prim, const glm::mat4& transform) {
    GltfPrimitive out;
    const auto& attrs = prim.at("attributes");
    const auto positions =
        ReadFloat3Accessor(gltf, bin, attrs.at("POSITION").get<int>());
    const auto normals = attrs.contains("NORMAL")
        ? ReadFloat3Accessor(gltf, bin, attrs.at("NORMAL").get<int>())
        : std::vector<glm::vec3>();
    const auto uvs = attrs.contains("TEXCOORD_0")
        ? ReadFloat2Accessor(gltf, bin, attrs.at("TEXCOORD_0").get<int>())
        : std::vector<glm::vec2>();

    // FS2024's landmark models place most meshes on child nodes with
    // their own translation/rotation/scale rather than baking it into
    // the mesh data, so accessor-space positions/normals mean nothing
    // until `transform` (that node's world transform) is applied.
    const glm::mat3 normalTransform(transform);
    out.mesh.vertices.resize(positions.size());
    for (std::size_t i = 0; i < positions.size(); ++i) {
        BspRenderVertex& v = out.mesh.vertices[i];
        const glm::vec3 pos(transform * glm::vec4(positions[i], 1.f));
        v.x = pos.x; v.y = pos.y; v.z = pos.z;
        v.u = i < uvs.size() ? uvs[i].x : 0.f;
        v.v = i < uvs.size() ? uvs[i].y : 0.f;
        v.lm_u = v.lm_v = 0.f;
        const glm::vec3 normal = i < normals.size()
            ? glm::normalize(normalTransform * normals[i])
            : glm::vec3(0.f);
        v.nx = normal.x; v.ny = normal.y; v.nz = normal.z;
    }
    if (prim.contains("indices")) {
        auto allIndices =
            ReadIndexAccessor(gltf, bin, prim.at("indices").get<int>());
        // FS2024 packs every primitive of a mesh that shares vertex data
        // into ONE "indices" accessor and tells them apart with its own
        // `ASOBO_primitive` extra: `StartIndex`/`PrimitiveCount` (index
        // elements/triangles) name each primitive's own slice, since
        // core glTF has no notion of a shared accessor's sub-range.
        // Skipping this and taking the whole accessor, as ordinary glTF
        // would, draws every primitive with all of the mesh's geometry
        // -- every material overlapping the same full shape.
        const auto& asobo =
            prim.value("extras", json::object())
                .value("ASOBO_primitive", json::object());
        if (asobo.contains("PrimitiveCount")) {
            const auto start = std::min(
                asobo.value("StartIndex", std::size_t{0}), allIndices.size());
            const auto end = std::min(
                start + asobo.at("PrimitiveCount").get<std::size_t>() * 3,
                allIndices.size());
            out.mesh.indices.assign(allIndices.begin() + start,
                                   allIndices.begin() + end);
            const int baseVertex = asobo.value("BaseVertexIndex", 0);
            if (baseVertex != 0) {
                for (auto& index : out.mesh.indices) {
                    index += static_cast<std::uint32_t>(baseVertex);
                }
            }
        } else {
            out.mesh.indices = std::move(allIndices);
        }
    }
    if (!prim.contains("material")) return out;
    const auto& mat = gltf.at("materials").at(prim.at("material").get<int>());
    if (!mat.contains("pbrMetallicRoughness")) return out;
    const auto& pbr = mat.at("pbrMetallicRoughness");
    if (!pbr.contains("baseColorTexture")) return out;
    const int texIndex = pbr.at("baseColorTexture").at("index").get<int>();
    const auto& tex = gltf.at("textures").at(texIndex);
    // FS2024's DDS textures aren't a core glTF image type, so
    // `MSFT_texture_dds` carries the real image index instead of the
    // plain `source` a PNG/JPEG texture would use.
    const int imgIndex = tex.contains("extensions") &&
                        tex.at("extensions").contains("MSFT_texture_dds")
        ? tex.at("extensions").at("MSFT_texture_dds").at("source").get<int>()
        : tex.value("source", -1);
    if (imgIndex < 0) return out;
    out.baseColorImageUri = gltf.at("images").at(imgIndex).value("uri", "");
    return out;
}

GltfLod ParseLodGlb(const std::uint8_t* data, std::size_t size) {
    const auto split = SplitGlb(data, size);
    std::string jsonText(reinterpret_cast<const char*>(split.json),
                         split.jsonLen);
    while (!jsonText.empty() &&
          (jsonText.back() == '\0' || jsonText.back() == ' ')) {
        jsonText.pop_back();
    }
    const json gltf = json::parse(jsonText);
    const auto transforms = ComputeNodeWorldTransforms(gltf);

    GltfLod lod;
    const auto& nodes = gltf.at("nodes");
    for (std::size_t i = 0; i < nodes.size(); ++i) {
        if (!nodes[i].contains("mesh")) continue;
        const auto& mesh = gltf.at("meshes").at(nodes[i].at("mesh").get<int>());
        for (const auto& prim : mesh.at("primitives")) {
            lod.primitives.push_back(
                BuildPrimitive(gltf, split.bin, prim, transforms[i]));
        }
    }
    return lod;
}

}  // namespace

std::vector<GltfLod> ParseModelRiff(const std::vector<std::uint8_t>& riff) {
    if (riff.size() < 12 || std::memcmp(riff.data(), "RIFF", 4) != 0) {
        throw std::runtime_error("model RIFF: bad magic");
    }
    const auto topChunks = WalkRiffChunks(riff.data() + 12, riff.size() - 12);
    std::string gxml;
    const RiffChunk* glbd = nullptr;
    for (const auto& chunk : topChunks) {
        if (chunk.id == "GXML") {
            gxml.assign(reinterpret_cast<const char*>(chunk.data),
                       chunk.size);
        } else if (chunk.id == "GLBD") {
            glbd = &chunk;
        }
    }
    if (!glbd) return {};

    const auto lodHeaders = ParseGxmlLods(gxml);
    const auto glbChunks = WalkRiffChunks(glbd->data, glbd->size);
    std::vector<GltfLod> lods;
    for (std::size_t i = 0; i < glbChunks.size(); ++i) {
        GltfLod lod = ParseLodGlb(glbChunks[i].data, glbChunks[i].size);
        if (i < lodHeaders.size()) {
            lod.modelFile = lodHeaders[i].modelFile;
            lod.minSize = lodHeaders[i].minSize;
        }
        lods.push_back(std::move(lod));
    }
    return lods;
}

}  // namespace sdl3cpp::tools::fs2024
