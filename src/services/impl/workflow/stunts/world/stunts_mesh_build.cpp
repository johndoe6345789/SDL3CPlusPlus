#include "services/interfaces/workflow/stunts/world/stunts_mesh_build.hpp"

#include <cmath>

namespace sdl3cpp::services::impl {
namespace {

PlanePosUvVertex Vertex(const glm::vec3& at, float u, float v) {
    return PlanePosUvVertex{at.x, at.y, at.z, u, v};
}

}  // namespace

void AppendStuntsQuad(GeometryPlaneMesh& mesh, const glm::vec3& a,
                      const glm::vec3& b, const glm::vec3& c,
                      const glm::vec3& d, float vRepeat) {
    const auto base = static_cast<std::uint16_t>(mesh.vertices.size());
    mesh.vertices.push_back(Vertex(a, 0.f, 0.f));
    mesh.vertices.push_back(Vertex(b, 1.f, 0.f));
    mesh.vertices.push_back(Vertex(c, 1.f, vRepeat));
    mesh.vertices.push_back(Vertex(d, 0.f, vRepeat));
    const std::uint16_t order[6] = {0, 1, 2, 0, 2, 3};
    for (const std::uint16_t offset : order) {
        mesh.indices.push_back(static_cast<std::uint16_t>(base + offset));
    }
}

glm::vec3 StuntsLinkDirection(std::uint8_t link) {
    switch (link) {
        case kStuntsLinkNorth: return {0.f, 0.f, -1.f};
        case kStuntsLinkSouth: return {0.f, 0.f, 1.f};
        case kStuntsLinkEast: return {1.f, 0.f, 0.f};
        case kStuntsLinkWest: return {-1.f, 0.f, 0.f};
        default: return {0.f, 0.f, 0.f};
    }
}

void AppendStuntsArm(GeometryPlaneMesh& mesh, const glm::vec3& centre,
                     std::uint8_t link, float tileSize, float width) {
    const glm::vec3 along = StuntsLinkDirection(link);
    if (along == glm::vec3(0.f)) return;
    const glm::vec3 across = glm::vec3(along.z, 0.f, -along.x) * (width * 0.5f);
    const glm::vec3 edge = centre + along * (tileSize * 0.5f);
    AppendStuntsQuad(mesh, centre - across, centre + across, edge + across,
                     edge - across, 0.5f);
}

}  // namespace sdl3cpp::services::impl
