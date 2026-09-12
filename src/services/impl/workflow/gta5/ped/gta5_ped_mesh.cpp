#include "services/interfaces/workflow/gta5/ped/gta5_ped.hpp"

#include "services/interfaces/workflow/gta5/ped/gta5_ped_frame.hpp"
#include "services/interfaces/workflow/gta5/render/gta5_shader_surface.hpp"
#include "services/interfaces/workflow/gta5/render/gta5_vertex_layout.hpp"

namespace sdl3cpp::services::impl {
namespace {

constexpr int kWeights = 16;  // R8G8B8A8_UNORM
constexpr int kIndices = 20;  // R8G8B8A8_UINT, into the geometry's bone ids

bool ReadPart(const Gta5Resource& res, std::int64_t geometry,
              Gta5PedPart& part) {
    const std::int64_t buffer = res.Follow(geometry + 0x18);
    Gta5VertexLayout layout;
    if (!ReadGta5VertexLayout(res, buffer, layout) ||
        layout.count >= kGta5MaxVerticesPerMesh) {
        return false;
    }
    const std::int64_t decl = res.Follow(buffer + 0x38);
    const std::int64_t ids = res.Follow(geometry + 0x68);
    const std::uint32_t count = res.U32(geometry + 0x58);
    const std::int64_t held = res.Follow(geometry + 0x38);
    const std::int64_t index = held < 0 ? -1 : res.Follow(held + 0x18);
    if (count == 0 || index < 0 || ids < 0) return false;
    for (std::uint32_t i = 0; i < count; ++i) {
        const std::uint16_t v = res.U16(index + 2 * std::int64_t{i});
        if (v >= layout.count) return false;
        part.mesh.indices.push_back(v);
    }
    for (std::uint32_t i = 0; i < layout.count; ++i) {
        const std::int64_t at = layout.data + std::int64_t{i} * layout.stride;
        part.mesh.vertices.push_back(ReadGta5Vertex(res, layout, i));
        ReadGta5PedBlend(res, at + res.U32(decl + 4 * kWeights),
                         at + res.U32(decl + 4 * kIndices), ids,
                         res.U16(geometry + 0x72), part);
    }
    return true;
}

}  // namespace

std::vector<Gta5PedPart> ReadGta5PedParts(const Gta5Resource& res,
                                          std::int64_t drawable) {
    std::vector<Gta5PedPart> parts;
    const auto surfaces = ReadGta5ShaderSurfaces(res, drawable);
    const std::int64_t models = res.Follow(drawable + 0x50);
    if (models < 0) return parts;
    for (const std::int64_t model : res.PointerList(models)) {
        const auto geometries = res.PointerList(model + 0x08);
        const std::int64_t shaders = res.Follow(model + 0x20);
        for (std::size_t g = 0; model >= 0 && g < geometries.size(); ++g) {
            Gta5PedPart part;
            if (!ReadPart(res, geometries[g], part)) continue;
            const std::size_t s = shaders < 0 ? surfaces.size()
                                  : res.U16(shaders + 2 * std::int64_t(g));
            if (s < surfaces.size()) {
                part.mesh.textureHash = surfaces[s].texture;
                part.mesh.alphaCutoff = surfaces[s].cutout ? 0.5f : 0.f;
                part.mesh.blend = surfaces[s].blend;
            }
            parts.push_back(std::move(part));
        }
    }
    return parts;
}

}  // namespace sdl3cpp::services::impl
