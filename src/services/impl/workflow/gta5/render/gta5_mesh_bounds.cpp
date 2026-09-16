#include "services/interfaces/workflow/gta5/render/gta5_mesh_bounds.hpp"

#include <glm/glm.hpp>

#include <algorithm>
#include <cmath>

namespace sdl3cpp::services::impl {

std::array<float, 4> Gta5MeshBounds(const Gta5MeshData& mesh) {
    glm::vec3 lo(1e30f), hi(-1e30f);
    for (const Gta5SubMeshData& part : mesh.parts) {
        for (const BspRenderVertex& v : part.vertices) {
            lo = glm::min(lo, glm::vec3(v.x, v.y, v.z));
            hi = glm::max(hi, glm::vec3(v.x, v.y, v.z));
        }
    }
    if (lo.x > hi.x) return {0.f, 0.f, 0.f, -1.f};
    const glm::vec3 c = (lo + hi) * 0.5f;
    float r2          = 0.f;
    for (const Gta5SubMeshData& part : mesh.parts) {
        for (const BspRenderVertex& v : part.vertices) {
            const glm::vec3 d = glm::vec3(v.x, v.y, v.z) - c;
            r2                = std::max(r2, glm::dot(d, d));
        }
    }
    return {c.x, c.y, c.z, std::sqrt(r2)};
}

}  // namespace sdl3cpp::services::impl
