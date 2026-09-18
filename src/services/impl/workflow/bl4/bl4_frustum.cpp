#include "services/interfaces/workflow/bl4/bl4_frustum.hpp"

namespace sdl3cpp::services::impl {

Bl4Frustum MakeBl4Frustum(const glm::mat4& viewProj) {
    // Gribb and Hartmann: each plane is the last row of the matrix plus
    // or minus another. Its rows are the columns of the transpose.
    const glm::mat4 rows = glm::transpose(viewProj);
    Bl4Frustum frustum;
    frustum.planes = {rows[3] + rows[0], rows[3] - rows[0],
                      rows[3] + rows[1], rows[3] - rows[1],
                      rows[3] + rows[2], rows[3] - rows[2]};
    for (glm::vec4& plane : frustum.planes) {
        plane /= glm::length(glm::vec3(plane));
    }
    return frustum;
}

bool Bl4InstanceVisible(const Bl4Frustum& frustum, const Bl4Instance& instance,
                        const glm::vec3& camera, float sizeRatio) {
    if (instance.boundsRadius < 0.f) return true;
    const float radius = instance.boundsRadius;
    if (radius < sizeRatio * glm::distance(instance.boundsCenter, camera)) {
        return false;
    }
    for (const glm::vec4& plane : frustum.planes) {
        if (glm::dot(glm::vec3(plane), instance.boundsCenter) + plane.w < -radius) {
            return false;
        }
    }
    return true;
}

}  // namespace sdl3cpp::services::impl
