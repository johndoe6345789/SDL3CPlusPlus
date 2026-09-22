#include "services/interfaces/workflow/stunts/data/stunts_shape_mesh.hpp"

#include <algorithm>

namespace sdl3cpp::services::impl {
namespace {

glm::vec3 ToPosition(const StuntsShapeVertex& v) {
    return glm::vec3(v.x, v.y, v.z) * kStuntsUnitsToMetres;
}

float PaletteU(std::uint8_t materialId) {
    return (static_cast<float>(materialId) + 0.5f) / 256.f;
}

void AppendFace(GeometryPlaneMesh& mesh, const StuntsShape& shape,
                const StuntsShapeFace& face, std::size_t paintJob) {
    if (face.kind != StuntsPrimitiveKind::Polygon || face.indices.size() < 3) {
        return;
    }
    const std::size_t column =
        face.materials.empty()
            ? 0
            : std::min(paintJob, face.materials.size() - 1);
    const float u = PaletteU(face.materials.empty() ? 0
                                                     : face.materials[column]);
    const auto base = static_cast<std::uint16_t>(mesh.vertices.size());
    for (const std::uint8_t index : face.indices) {
        const glm::vec3 pos = ToPosition(shape.vertices[index]);
        mesh.vertices.push_back(
            PlanePosUvVertex{pos.x, pos.y, pos.z, u, kStuntsPaletteV});
    }
    // Fan-triangulate: vertex 0 with every consecutive edge. Valid
    // for the convex faces flat-shaded polygon shapes are built from.
    for (std::size_t i = 1; i + 1 < face.indices.size(); ++i) {
        mesh.indices.push_back(base);
        mesh.indices.push_back(static_cast<std::uint16_t>(base + i));
        mesh.indices.push_back(static_cast<std::uint16_t>(base + i + 1));
    }
}

}  // namespace

GeometryPlaneMesh BuildStuntsShapeMesh(const StuntsShape& shape,
                                       std::size_t paintJob) {
    GeometryPlaneMesh mesh;
    if (!shape.valid) return mesh;
    for (const StuntsShapeFace& face : shape.faces) {
        AppendFace(mesh, shape, face, paintJob);
    }
    return mesh;
}

}  // namespace sdl3cpp::services::impl
