#include "services/interfaces/workflow/particles/particle_billboards.hpp"

#include <algorithm>

namespace sdl3cpp::services::impl {

namespace {

BspRenderVertex MakeCorner(const glm::vec3& at, const glm::vec3& normal,
                           float u, float v) {
    BspRenderVertex vertex{};
    vertex.x = at.x;
    vertex.y = at.y;
    vertex.z = at.z;
    vertex.u = u;
    vertex.v = v;
    vertex.nx = normal.x;
    vertex.ny = normal.y;
    vertex.nz = normal.z;
    return vertex;
}

}  // namespace

void AppendParticleBillboards(const std::vector<Particle>& particles,
                              const glm::vec3& right, const glm::vec3& up,
                              std::vector<BspRenderVertex>& out) {
    const glm::vec3 normal = glm::normalize(glm::cross(right, up));
    for (const Particle& particle : particles) {
        const float half =
            std::max(0.0f, particle.size * (1.0f - particle.Progress())) * 0.5f;
        if (half <= 0.0f) {
            continue;
        }
        const glm::vec3 across = right * half;
        const glm::vec3 down = up * half;
        const glm::vec3 topLeft = particle.position - across + down;
        const glm::vec3 topRight = particle.position + across + down;
        const glm::vec3 bottomLeft = particle.position - across - down;
        const glm::vec3 bottomRight = particle.position + across - down;

        out.push_back(MakeCorner(topLeft, normal, 0.0f, 0.0f));
        out.push_back(MakeCorner(bottomLeft, normal, 0.0f, 1.0f));
        out.push_back(MakeCorner(topRight, normal, 1.0f, 0.0f));
        out.push_back(MakeCorner(topRight, normal, 1.0f, 0.0f));
        out.push_back(MakeCorner(bottomLeft, normal, 0.0f, 1.0f));
        out.push_back(MakeCorner(bottomRight, normal, 1.0f, 1.0f));
    }
}

}  // namespace sdl3cpp::services::impl
