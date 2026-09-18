#include "services/interfaces/workflow/fs2024/data/gltf/fs2024_gltf_primitive.hpp"

#include "services/interfaces/workflow/fs2024/data/gltf/fs2024_gltf_accessor.hpp"

#include <glm/mat3x3.hpp>
#include <glm/vec4.hpp>

namespace sdl3cpp::fs2024 {

using nlohmann::json;
using services::impl::BspRenderVertex;

GltfPrimitive BuildGltfPrimitive(const json& gltf, const std::uint8_t* bin,
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
    out.mesh.indices = ReadGltfPrimitiveIndices(gltf, bin, prim);
    out.baseColorImageUri = GltfBaseColorImageUri(gltf, prim);
    return out;
}

}  // namespace sdl3cpp::fs2024
