#include "services/interfaces/workflow/bl4/bl4_binary_mesh.hpp"

#include <cmath>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <vector>

namespace sdl3cpp::services::impl {
namespace {

bool Read(std::ifstream& in, void* into, std::size_t bytes) {
    in.read(static_cast<char*>(into), static_cast<std::streamsize>(bytes));
    return static_cast<bool>(in);
}

std::uint32_t ReadU32(std::ifstream& in, bool& ok) {
    std::uint32_t value = 0;
    ok = ok && Read(in, &value, sizeof(value));
    return value;
}

}  // namespace

Bl4MeshData LoadBl4BinaryMesh(const std::string& path) {
    Bl4MeshData out;
    std::ifstream in(path, std::ios::binary);
    if (!in) return out;

    char magic[4] = {};
    bool ok = Read(in, magic, sizeof(magic));
    if (!ok || std::memcmp(magic, "BL4M", 4) != 0) return out;
    const std::uint32_t version = ReadU32(in, ok);
    const std::uint32_t parts = ReadU32(in, ok);
    if (!ok || version != 1) return out;

    float bounds[6] = {};
    if (!Read(in, bounds, sizeof(bounds))) return out;

    for (std::uint32_t p = 0; p < parts && ok; ++p) {
        const std::uint32_t vertexCount = ReadU32(in, ok);
        const std::uint32_t indexCount = ReadU32(in, ok);
        const std::uint32_t textureLength = ReadU32(in, ok);
        if (!ok) break;
        Bl4SubMeshData part;
        part.texturePath.resize(textureLength);
        if (textureLength > 0 && !Read(in, part.texturePath.data(), textureLength)) return {};
        part.vertices.resize(vertexCount);
        if (vertexCount > 0 &&
            !Read(in, part.vertices.data(), vertexCount * sizeof(BspRenderVertex))) {
            return {};
        }
        part.indices.resize(indexCount);
        if (indexCount > 0 && !Read(in, part.indices.data(), indexCount * sizeof(std::uint32_t))) {
            return {};
        }
        if (!part.indices.empty()) out.parts.push_back(std::move(part));
    }
    if (out.parts.empty()) return {};
    for (int i = 0; i < 3; ++i) {
        out.boundsCenter[i] = (bounds[i] + bounds[i + 3]) * 0.5f;
    }
    float radius = 0.f;
    for (int i = 0; i < 3; ++i) {
        const float half = (bounds[i + 3] - bounds[i]) * 0.5f;
        radius += half * half;
    }
    // The file's box, as a sphere: slightly looser than the per-vertex
    // radius assimp's path computes, which only costs a little culling.
    out.boundsRadius = radius > 0.f ? std::sqrt(radius) : 0.f;
    return out;
}

}  // namespace sdl3cpp::services::impl
