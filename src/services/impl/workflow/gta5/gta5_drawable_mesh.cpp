#include "services/interfaces/workflow/gta5/gta5_drawable_mesh.hpp"

#include "services/interfaces/workflow/gta5/gta5_geometry.hpp"
#include "services/interfaces/workflow/gta5/gta5_shader_surface.hpp"
#include "services/interfaces/workflow/gta5/gta5_vertex_layout.hpp"

namespace sdl3cpp::services::impl {
namespace {

bool ReadGeometry(const Gta5Resource& res, std::int64_t geometry,
                  Gta5SubMeshData& part) {
    Gta5VertexLayout layout;
    if (!ReadGta5VertexLayout(res, res.Follow(geometry + 0x18), layout) ||
        layout.count >= kGta5MaxVerticesPerMesh) {
        return false;
    }
    const std::uint32_t count = res.U32(geometry + 0x58);
    const std::int64_t buffer = res.Follow(geometry + 0x38);
    const std::int64_t indices = buffer < 0 ? -1 : res.Follow(buffer + 0x18);
    if (count == 0 || indices < 0 ||
        static_cast<std::uint64_t>(indices) + 2ull * count > res.data.size()) {
        return false;
    }
    part.indices.resize(count);
    for (std::uint32_t i = 0; i < count; ++i) {
        const std::uint16_t index = res.U16(indices + 2 * std::int64_t{i});
        if (index >= layout.count) return false;  // a bad read, not a mesh
        part.indices[i] = index;
    }
    part.vertices.reserve(layout.count);
    for (std::uint32_t i = 0; i < layout.count; ++i) {
        part.vertices.push_back(ReadGta5Vertex(res, layout, i));
    }
    return true;
}

}  // namespace

Gta5MeshData ReadGta5DrawableMesh(const Gta5Resource& res,
                                  std::int64_t drawable,
                                  const glm::vec3& paint) {
    Gta5MeshData mesh;
    const auto surfaces = ReadGta5ShaderSurfaces(res, drawable);
    const std::int64_t models = res.Follow(drawable + 0x50);
    if (models < 0) return mesh;
    for (const std::int64_t model : res.PointerList(models)) {
        if (model < 0) continue;
        const auto geometries = res.PointerList(model + 0x08);
        const std::int64_t shaders = res.Follow(model + 0x20);
        for (std::size_t g = 0; g < geometries.size(); ++g) {
            Gta5SubMeshData part;
            if (!ReadGeometry(res, geometries[g], part)) continue;
            const std::size_t s =
                shaders < 0 ? surfaces.size()
                            : res.U16(shaders + 2 * std::int64_t(g));
            if (s < surfaces.size()) {
                part.textureHash = surfaces[s].texture;
                if (surfaces[s].paint) part.tint = {paint.r, paint.g, paint.b};
                if (surfaces[s].cutout) part.alphaCutoff = 0.5f;
            }
            mesh.parts.push_back(std::move(part));
        }
    }
    return mesh;
}

}  // namespace sdl3cpp::services::impl
