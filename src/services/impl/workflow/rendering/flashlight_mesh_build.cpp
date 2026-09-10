#include "services/interfaces/workflow/rendering/flashlight_mesh_internal.hpp"

namespace sdl3cpp::services::impl {

FlashlightMesh BuildFlashlightMesh(int segments, float bodyRadius,
                                   float bodyLength, float headRadius,
                                   float headLength, float lensRadius) {
    namespace detail = flashlight_mesh_detail;

    FlashlightMesh mesh;
    auto& v   = mesh.vertices;
    auto& idx = mesh.indices;

    // Body: long cylinder (handle/grip)
    // bottom cap
    detail::AddCap(v, idx, segments, bodyRadius, 0.0f, 0.0f, true);
    detail::AddCylinder(v, idx, segments, bodyRadius, bodyRadius, 0.0f,
                        bodyLength, 0.0f, 0.6f);

    // Head: slightly wider cylinder (where the bulb sits)
    detail::AddCylinder(v, idx, segments, bodyRadius, headRadius, bodyLength,
                        bodyLength + 0.02f, 0.6f, 0.7f);
    detail::AddCylinder(v, idx, segments, headRadius, headRadius,
                        bodyLength + 0.02f, bodyLength + headLength, 0.7f,
                        0.9f);

    // Lens: flat disc at the front (the light-emitting surface)
    mesh.lensY = bodyLength + headLength;
    detail::AddCap(v, idx, segments, lensRadius, mesh.lensY, 1.0f, false);

    return mesh;
}

}  // namespace sdl3cpp::services::impl
