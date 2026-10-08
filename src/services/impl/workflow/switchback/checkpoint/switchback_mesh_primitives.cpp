#include "services/interfaces/workflow/switchback/checkpoint/switchback_mesh_primitives.hpp"

#include <cstdint>

namespace sdl3cpp::services::impl {

void AddSwitchbackQuad(GeometryPlaneMesh& mesh, const glm::vec3& a,
                       const glm::vec3& b, const glm::vec3& c,
                       const glm::vec3& d, float shade, float material) {
    static constexpr std::uint16_t kCorners[6] = {0, 1, 2, 0, 2, 3};
    const auto first = static_cast<std::uint16_t>(mesh.vertices.size());
    const glm::vec3 corners[4] = {a, b, c, d};
    for (const glm::vec3& p : corners) {
        mesh.vertices.push_back({p.x, p.y, p.z, shade, material});
    }
    for (std::uint16_t corner : kCorners) {
        mesh.indices.push_back(static_cast<std::uint16_t>(first + corner));
    }
}

void AddSwitchbackBox(GeometryPlaneMesh& mesh, const glm::vec3& centre,
                      const SwitchbackAxes& axes, const glm::vec3& half,
                      float material) {
    glm::vec3 c[8];
    for (int i = 0; i < 8; ++i) {
        const float sx = (i & 1) ? 1.f : -1.f;
        const float sy = (i & 2) ? 1.f : -1.f;
        const float sz = (i & 4) ? 1.f : -1.f;
        c[i] = centre + axes.across * (sx * half.x) +
               axes.up * (sy * half.y) + axes.along * (sz * half.z);
    }
    static constexpr int kFaces[6][4] = {{0, 1, 3, 2}, {4, 5, 7, 6},
                                         {0, 2, 6, 4}, {1, 3, 7, 5},
                                         {0, 1, 5, 4}, {2, 3, 7, 6}};
    static constexpr float kShades[6] = {0.6f, 0.6f, 0.6f,
                                         0.6f, 0.3f, 1.0f};
    for (int face = 0; face < 6; ++face) {
        AddSwitchbackQuad(mesh, c[kFaces[face][0]], c[kFaces[face][1]],
                          c[kFaces[face][2]], c[kFaces[face][3]],
                          kShades[face], material);
    }
}

}  // namespace sdl3cpp::services::impl
