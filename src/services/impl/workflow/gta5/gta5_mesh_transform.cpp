#include "services/interfaces/workflow/gta5/gta5_mesh_transform.hpp"

#include <algorithm>
#include <cmath>
#include <utility>

namespace sdl3cpp::services::impl {

void TurnGta5MeshAround(Gta5MeshData& mesh) {
    for (Gta5SubMeshData& part : mesh.parts) {
        for (BspRenderVertex& v : part.vertices) {
            v.x = -v.x;
            v.z = -v.z;
            v.nx = -v.nx;
            v.nz = -v.nz;
        }
    }
}

Gta5MeshData ShapeGta5Wheel(const Gta5MeshData& wheel, float radius,
                            float width, bool mirror) {
    float radial = 0.f;
    float axial = 0.f;
    for (const Gta5SubMeshData& part : wheel.parts) {
        for (const BspRenderVertex& v : part.vertices) {
            radial = std::max(radial, std::hypot(v.y, v.z));
            axial = std::max(axial, std::abs(v.x));
        }
    }
    Gta5MeshData out = wheel;
    if (radial <= 0.f || axial <= 0.f) return out;

    const float across = (width * 0.5f / axial) * (mirror ? -1.f : 1.f);
    const float around = radius / radial;
    for (Gta5SubMeshData& part : out.parts) {
        for (BspRenderVertex& v : part.vertices) {
            v.x *= across;
            v.y *= around;
            v.z *= around;
            if (mirror) v.nx = -v.nx;
        }
        if (!mirror) continue;
        for (std::size_t t = 0; t + 2 < part.indices.size(); t += 3) {
            std::swap(part.indices[t + 1], part.indices[t + 2]);
        }
    }
    return out;
}

}  // namespace sdl3cpp::services::impl
