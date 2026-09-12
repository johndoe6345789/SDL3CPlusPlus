#include "services/interfaces/workflow/gta5/effects/gta5_effects_quad.hpp"

#include <cmath>

namespace sdl3cpp::services::impl {
namespace {

constexpr float kCells = 4.f;   // the drawn strip is four sprites across
constexpr float kDecalX = 8.f;  // GTA's sheet is eight marks across
constexpr float kDecalY = 4.f;  // and four down

void Corner(std::vector<BspRenderVertex>& out, const glm::vec3& at,
            const Gta5Particle& p, float u, float v, float alpha) {
    BspRenderVertex vertex{};
    vertex.x = at.x, vertex.y = at.y, vertex.z = at.z;
    if (p.cell >= 0) {
        // Inside the cell by a hair, so the filter does not fetch the
        // mark next door.
        const float col = float(p.cell % int(kDecalX));
        const float row = float((p.cell / int(kDecalX)) % int(kDecalY));
        vertex.u = (col + 0.02f + u * 0.96f) / kDecalX;
        vertex.v = (row + 0.02f + v * 0.96f) / kDecalY;
    } else {
        vertex.u = (float(p.sprite) + u) / kCells;
        vertex.v = v;
    }
    vertex.nx = p.colour.r, vertex.ny = p.colour.g, vertex.nz = p.colour.b;
    vertex.lm_u = alpha;
    out.push_back(vertex);
}

}  // namespace

void AppendGta5Quad(std::vector<BspRenderVertex>& out,
                    const Gta5Particle& p, const glm::vec3& centre,
                    const glm::vec3& across, const glm::vec3& down,
                    float alpha) {
    Corner(out, centre - across - down, p, 0.f, 0.f, alpha);
    Corner(out, centre + across - down, p, 1.f, 0.f, alpha);
    Corner(out, centre + across + down, p, 1.f, 1.f, alpha);
    Corner(out, centre - across - down, p, 0.f, 0.f, alpha);
    Corner(out, centre + across + down, p, 1.f, 1.f, alpha);
    Corner(out, centre - across + down, p, 0.f, 1.f, alpha);
}

void Gta5DecalAxes(const glm::vec3& normal, glm::vec3& right,
                   glm::vec3& ahead) {
    const glm::vec3 other =
        std::abs(normal.y) > 0.9f ? glm::vec3(1.f, 0.f, 0.f)
                                  : glm::vec3(0.f, 1.f, 0.f);
    right = glm::normalize(glm::cross(other, normal));
    ahead = glm::cross(normal, right);
}

}  // namespace sdl3cpp::services::impl
