#include "services/interfaces/workflow/racer/world/racer_pod_rig.hpp"

#include <cmath>

namespace sdl3cpp::services::impl {
namespace {

RacerGpuVertex Vertex(const glm::vec3& p, const glm::vec4& c) {
    return {p.x, p.y, p.z, 0.5f, 0.5f, c.a, 0.f, c.r, c.g, c.b};
}

}  // namespace

std::vector<RacerGpuVertex> RacerConeVertices(int segments,
                                              const glm::vec4& base,
                                              const glm::vec4& tip) {
    // Radius 1 at z = 0, a point at z = 1: scaled per draw.
    std::vector<RacerGpuVertex> out;
    const glm::vec3 point(0.f, 0.f, 1.f);
    for (int s = 0; s < segments; ++s) {
        const float a0 = 6.2831853f * s / segments;
        const float a1 = 6.2831853f * (s + 1) / segments;
        const glm::vec3 p0(std::cos(a0), std::sin(a0), 0.f);
        const glm::vec3 p1(std::cos(a1), std::sin(a1), 0.f);
        out.insert(out.end(), {Vertex(p0, base), Vertex(p1, base),
                               Vertex(point, tip)});
    }
    return out;
}

std::vector<RacerGpuVertex> RacerBeamVertices(const glm::vec3& from,
                                              const glm::vec3& to,
                                              float width,
                                              const glm::vec4& colour) {
    // Two crossed quads along the beam, so it reads from any side.
    const glm::vec3 along = to - from;
    glm::vec3 side = glm::cross(along, glm::vec3(0.f, 1.f, 0.f));
    if (glm::length(side) < 1e-5f) side = glm::vec3(1.f, 0.f, 0.f);
    side = glm::normalize(side) * (0.5f * width);
    const glm::vec3 up =
        glm::normalize(glm::cross(side, along)) * (0.5f * width);
    std::vector<RacerGpuVertex> out;
    for (const glm::vec3& o : {side, up}) {
        const glm::vec3 a = from - o, b = from + o, c = to + o, d = to - o;
        out.insert(out.end(), {Vertex(a, colour), Vertex(b, colour),
                               Vertex(c, colour), Vertex(a, colour),
                               Vertex(c, colour), Vertex(d, colour)});
    }
    return out;
}

}  // namespace sdl3cpp::services::impl
