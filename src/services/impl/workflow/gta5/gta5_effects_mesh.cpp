#include "services/interfaces/workflow/gta5/gta5_effects_quad.hpp"

#include <algorithm>

namespace sdl3cpp::services::impl {
namespace {

/// One piece's corners, or nothing when it has faded out.
void Piece(std::vector<BspRenderVertex>& out, const Gta5Particle& p,
           const glm::vec3& right, const glm::vec3& up) {
    const float share = p.life > 0.f ? p.age / p.life : 1.f;
    // A mark holds its colour and goes in its last quarter; the rest
    // fade away as they age.
    const float left = p.normal != glm::vec3(0.f) && p.length == 0.f
                           ? std::min(1.f, (1.f - share) * 4.f)
                           : 1.f - share;
    const float alpha = p.fade * std::max(0.f, left);
    const float size = p.size + p.growth * p.age;
    if (alpha <= 0.003f) return;
    if (p.length > 0.f) {
        // A streak: along its own line, a hair wide across the view.
        const glm::vec3 along = p.normal * (p.length * 0.5f);
        const glm::vec3 wide =
            glm::normalize(glm::cross(p.normal, right - up)) * size;
        AppendGta5Quad(out, p, p.at + along, wide, along, alpha);
    } else if (p.normal != glm::vec3(0.f)) {
        glm::vec3 a, b;
        Gta5DecalAxes(p.normal, a, b);
        AppendGta5Quad(out, p, p.at, a * size, b * size, alpha);
    } else {
        AppendGta5Quad(out, p, p.at, right * size, up * size, alpha);
    }
}

}  // namespace

std::vector<BspRenderVertex> BuildGta5EffectQuads(const Gta5Effects& effects,
                                                  const glm::vec3& right,
                                                  const glm::vec3& up,
                                                  std::uint32_t& marks) {
    std::vector<BspRenderVertex> out;
    out.reserve(effects.particles.size() * 6);
    // The drawn strip first, then whatever wears a cell of GTA's sheet,
    // so each is drawn with its own texture bound.
    for (const Gta5Particle& p : effects.particles) {
        if (p.cell < 0) Piece(out, p, right, up);
    }
    const std::size_t strip = out.size();
    for (const Gta5Particle& p : effects.particles) {
        if (p.cell >= 0) Piece(out, p, right, up);
    }
    marks = static_cast<std::uint32_t>(out.size() - strip);
    return out;
}

}  // namespace sdl3cpp::services::impl
