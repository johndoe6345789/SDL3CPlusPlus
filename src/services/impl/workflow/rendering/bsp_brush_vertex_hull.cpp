#include "services/interfaces/workflow/rendering/bsp_brush_vertex_hull.hpp"

#include <cmath>

namespace sdl3cpp::services::impl {

std::vector<btVector3> ComputeBrushVertices(const BspBrushSide* sides,
                                            int numSides,
                                            const BspPlane* allPlanes,
                                            float scale) {
    std::vector<btVector3> verts;
    if (numSides < 3) return verts;

    struct Plane {
        btVector3 n;
        float d;
    };
    std::vector<Plane> planes(numSides);
    for (int i = 0; i < numSides; ++i) {
        const auto& p = allPlanes[sides[i].planeIndex];
        planes[i].n   = btVector3(p.normal[0], p.normal[2], -p.normal[1]);
        planes[i].d   = p.dist * scale;
    }

    for (int i = 0; i < numSides - 2; ++i) {
        for (int j = i + 1; j < numSides - 1; ++j) {
            for (int k = j + 1; k < numSides; ++k) {
                const auto& p1 = planes[i];
                const auto& p2 = planes[j];
                const auto& p3 = planes[k];

                btVector3 cross23 = p2.n.cross(p3.n);
                float denom       = p1.n.dot(cross23);
                if (std::fabs(denom) < 1e-6f) continue;

                btVector3 point = (cross23 * p1.d + p3.n.cross(p1.n) * p2.d +
                                   p1.n.cross(p2.n) * p3.d) /
                                  denom;

                bool inside = true;
                for (int m = 0; m < numSides; ++m) {
                    if (m == i || m == j || m == k) continue;
                    float dist = planes[m].n.dot(point) - planes[m].d;
                    if (dist > 0.1f * scale) {
                        inside = false;
                        break;
                    }
                }

                if (inside) {
                    verts.push_back(point);
                }
            }
        }
    }

    return verts;
}

}  // namespace sdl3cpp::services::impl
