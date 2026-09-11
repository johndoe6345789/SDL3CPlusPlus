#include "services/interfaces/workflow/gta5/gta5_effects.hpp"

#include <algorithm>
#include <cmath>

namespace sdl3cpp::services::impl {
namespace {

constexpr float kCells = 4.f;  // the atlas is four sprites across

void Corner(std::vector<BspRenderVertex>& out, const glm::vec3& at,
            const Gta5Particle& p, float u, float v, float alpha) {
    BspRenderVertex vertex{};
    vertex.x = at.x, vertex.y = at.y, vertex.z = at.z;
    vertex.u = (float(p.sprite) + u) / kCells;
    vertex.v = v;
    vertex.nx = p.colour.r, vertex.ny = p.colour.g, vertex.nz = p.colour.b;
    vertex.lm_u = alpha;
    out.push_back(vertex);
}

/// The four corners as two triangles, the quad's own axes scaled.
void Quad(std::vector<BspRenderVertex>& out, const Gta5Particle& p,
          const glm::vec3& centre, const glm::vec3& across,
          const glm::vec3& down, float alpha) {
    Corner(out, centre - across - down, p, 0.f, 0.f, alpha);
    Corner(out, centre + across - down, p, 1.f, 0.f, alpha);
    Corner(out, centre + across + down, p, 1.f, 1.f, alpha);
    Corner(out, centre - across - down, p, 0.f, 0.f, alpha);
    Corner(out, centre + across + down, p, 1.f, 1.f, alpha);
    Corner(out, centre - across + down, p, 0.f, 1.f, alpha);
}

/// Two directions across the surface a decal lies on.
void Across(const glm::vec3& normal, glm::vec3& right, glm::vec3& ahead) {
    const glm::vec3 other =
        std::abs(normal.y) > 0.9f ? glm::vec3(1.f, 0.f, 0.f)
                                  : glm::vec3(0.f, 1.f, 0.f);
    right = glm::normalize(glm::cross(other, normal));
    ahead = glm::cross(normal, right);
}

}  // namespace

std::vector<BspRenderVertex> BuildGta5EffectQuads(const Gta5Effects& effects,
                                                  const glm::vec3& right,
                                                  const glm::vec3& up) {
    std::vector<BspRenderVertex> out;
    out.reserve(effects.particles.size() * 6);
    for (const Gta5Particle& p : effects.particles) {
        const float share = p.life > 0.f ? p.age / p.life : 1.f;
        // A mark holds its colour and goes in its last quarter; the rest
        // fade away as they age.
        const float left = p.normal != glm::vec3(0.f) && p.length == 0.f
                               ? std::min(1.f, (1.f - share) * 4.f)
                               : 1.f - share;
        const float alpha = p.fade * std::max(0.f, left);
        const float size = p.size + p.growth * p.age;
        if (alpha <= 0.003f) continue;
        if (p.length > 0.f) {
            // A streak: along its own line, a hair wide across the view.
            const glm::vec3 along = p.normal * (p.length * 0.5f);
            const glm::vec3 wide =
                glm::normalize(glm::cross(p.normal, right - up)) * size;
            Quad(out, p, p.at + along, wide, along, alpha);
        } else if (p.normal != glm::vec3(0.f)) {
            glm::vec3 a, b;
            Across(p.normal, a, b);
            Quad(out, p, p.at, a * size, b * size, alpha);
        } else {
            Quad(out, p, p.at, right * size, up * size, alpha);
        }
    }
    return out;
}

}  // namespace sdl3cpp::services::impl
