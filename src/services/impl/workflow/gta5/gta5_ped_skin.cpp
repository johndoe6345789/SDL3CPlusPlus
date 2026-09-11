#include "services/interfaces/workflow/gta5/gta5_ped.hpp"

namespace sdl3cpp::services::impl {

void SkinGta5PedPart(const Gta5PedPart& part,
                     const std::vector<glm::mat4>& skin,
                     std::vector<BspRenderVertex>& out) {
    out = part.mesh.vertices;
    for (std::size_t i = 0; i < out.size(); ++i) {
        glm::mat4 blend(0.f);
        float total = 0.f;
        for (int k = 0; k < 4; ++k) {
            const float w = part.weights[i][k];
            const std::size_t bone = part.bones[i][k];
            if (w <= 0.f || bone >= skin.size()) continue;
            blend += skin[bone] * w;
            total += w;
        }
        if (total <= 0.f) continue;  // left in the bind pose
        BspRenderVertex& v = out[i];
        // Skinned in GTA's space: the engine's (x, y, z) is its (x, -z, y).
        const glm::vec4 p = blend * glm::vec4(v.x, -v.z, v.y, 1.f);
        const glm::vec3 n = glm::mat3(blend) * glm::vec3(v.nx, -v.nz, v.ny);
        v.x = p.x, v.y = p.z, v.z = -p.y;
        const float length = glm::length(n);
        if (length > 1e-6f) {
            v.nx = n.x / length, v.ny = n.z / length, v.nz = -n.y / length;
        }
    }
}

}  // namespace sdl3cpp::services::impl
