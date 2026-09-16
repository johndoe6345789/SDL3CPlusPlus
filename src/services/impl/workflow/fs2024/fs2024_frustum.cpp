#include "services/interfaces/workflow/fs2024/fs2024_frustum.hpp"

namespace sdl3cpp::services::impl {

Fs2024Frustum MakeFs2024Frustum(const glm::mat4& viewProj) {
    // Gribb and Hartmann: each plane is the last row of the matrix plus
    // or minus another. Its rows are the columns of the transpose.
    const glm::mat4 rows = glm::transpose(viewProj);
    Fs2024Frustum frustum;
    frustum.planes = {rows[3] + rows[0], rows[3] - rows[0],
                      rows[3] + rows[1], rows[3] - rows[1],
                      rows[3] + rows[2], rows[3] - rows[2]};
    for (glm::vec4& plane : frustum.planes) {
        plane /= glm::length(glm::vec3(plane));
    }
    return frustum;
}

bool Fs2024BoxVisible(const Fs2024Frustum& frustum, const glm::vec3& min,
                      const glm::vec3& max) {
    for (const glm::vec4& plane : frustum.planes) {
        // The corner furthest along the plane's normal: if even that is
        // behind the plane, the whole box is.
        const glm::vec3 far(plane.x >= 0.f ? max.x : min.x,
                            plane.y >= 0.f ? max.y : min.y,
                            plane.z >= 0.f ? max.z : min.z);
        if (glm::dot(glm::vec3(plane), far) + plane.w < 0.f) return false;
    }
    return true;
}

}  // namespace sdl3cpp::services::impl
