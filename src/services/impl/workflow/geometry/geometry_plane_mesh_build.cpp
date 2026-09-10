#include "services/interfaces/workflow/geometry/geometry_plane_helpers.hpp"

namespace sdl3cpp::services::impl {

GeometryPlaneMesh BuildGeometryPlaneMesh(const GeometryPlaneParams& params) {
    const float hw   = params.width * 0.5f;
    const float hd   = params.depth * 0.5f;
    const int vertsX = params.subdivisionsX + 1;
    const int vertsY = params.subdivisionsY + 1;

    GeometryPlaneMesh mesh;
    mesh.vertices.reserve(vertsX * vertsY);

    for (int iy = 0; iy < vertsY; ++iy) {
        float fy =
            static_cast<float>(iy) / static_cast<float>(params.subdivisionsY);
        for (int ix = 0; ix < vertsX; ++ix) {
            float fx = static_cast<float>(ix) /
                       static_cast<float>(params.subdivisionsX);
            PlanePosUvVertex v;
            v.x = -hw + fx * params.width;
            v.y = 0.0f;
            v.z = -hd + fy * params.depth;
            v.u = fx * params.uvScaleX;
            v.v = fy * params.uvScaleY;
            mesh.vertices.push_back(v);
        }
    }

    mesh.indices.reserve(params.subdivisionsX * params.subdivisionsY * 6);
    for (int iy = 0; iy < params.subdivisionsY; ++iy) {
        for (int ix = 0; ix < params.subdivisionsX; ++ix) {
            uint16_t tl = static_cast<uint16_t>(iy * vertsX + ix);
            uint16_t tr = tl + 1;
            uint16_t bl = static_cast<uint16_t>((iy + 1) * vertsX + ix);
            uint16_t br = bl + 1;

            mesh.indices.push_back(tl);
            mesh.indices.push_back(bl);
            mesh.indices.push_back(tr);
            mesh.indices.push_back(tr);
            mesh.indices.push_back(bl);
            mesh.indices.push_back(br);
        }
    }
    return mesh;
}

}  // namespace sdl3cpp::services::impl
