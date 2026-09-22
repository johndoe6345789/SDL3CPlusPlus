#include "services/interfaces/workflow/stunts/world/stunts_mesh_build.hpp"

#include <cmath>

namespace sdl3cpp::services::impl {

void AppendStuntsCorner(GeometryPlaneMesh& mesh, const glm::vec3& centre,
                        std::uint8_t links, float tileSize, float width,
                        int segments) {
    std::uint8_t first = kStuntsLinkNone;
    std::uint8_t second = kStuntsLinkNone;
    for (const std::uint8_t link : {kStuntsLinkNorth, kStuntsLinkEast,
                                    kStuntsLinkSouth, kStuntsLinkWest}) {
        if (!(links & link)) continue;
        (first == kStuntsLinkNone ? first : second) = link;
    }
    if (second == kStuntsLinkNone) return;

    // The turn is centred on the corner the two open edges share, so
    // its arc runs from one edge's midpoint to the other's.
    const glm::vec3 a = StuntsLinkDirection(first);
    const glm::vec3 b = StuntsLinkDirection(second);
    const glm::vec3 pivot = centre + (a + b) * (tileSize * 0.5f);
    const float radius = tileSize * 0.5f;
    const glm::vec3 from = -b;
    const glm::vec3 to = -a;
    const int steps = segments > 0 ? segments : 1;
    for (int step = 0; step < steps; ++step) {
        const float t0 = static_cast<float>(step) / static_cast<float>(steps);
        const float t1 =
            static_cast<float>(step + 1) / static_cast<float>(steps);
        const float a0 = t0 * 1.57079632679f;
        const float a1 = t1 * 1.57079632679f;
        const glm::vec3 d0 = from * std::cos(a0) + to * std::sin(a0);
        const glm::vec3 d1 = from * std::cos(a1) + to * std::sin(a1);
        AppendStuntsQuad(mesh, pivot + d0 * (radius - width * 0.5f),
                         pivot + d0 * (radius + width * 0.5f),
                         pivot + d1 * (radius + width * 0.5f),
                         pivot + d1 * (radius - width * 0.5f), 0.25f);
    }
}

}  // namespace sdl3cpp::services::impl
