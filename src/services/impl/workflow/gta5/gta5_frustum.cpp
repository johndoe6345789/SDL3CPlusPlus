#include "services/interfaces/workflow/gta5/gta5_frustum.hpp"

#include <glm/gtc/type_ptr.hpp>

#include <algorithm>
#include <cstring>

namespace sdl3cpp::services::impl {

Gta5Frustum MakeGta5Frustum(const glm::mat4& viewProj) {
    // Gribb and Hartmann: each plane is the last row of the matrix plus
    // or minus another. Its rows are the columns of the transpose.
    const glm::mat4 rows = glm::transpose(viewProj);
    Gta5Frustum frustum;
    frustum.planes = {rows[3] + rows[0], rows[3] - rows[0],
                      rows[3] + rows[1], rows[3] - rows[1],
                      rows[3] + rows[2], rows[3] - rows[2]};
    for (glm::vec4& plane : frustum.planes) {
        plane /= glm::length(glm::vec3(plane));
    }
    return frustum;
}

bool Gta5InstanceVisible(const Gta5Frustum& frustum,
                         const Gta5Instance& instance,
                         const glm::vec3& camera, float sizeRatio) {
    // GTA's LOD hierarchy: an entity is drawn out to its lodDist, and a
    // parent only beyond childLodDist, where its children hand over.
    // Drawn together they overlap, and fight for depth in the distance.
    const glm::vec3 origin(instance.modelMatrix[12], instance.modelMatrix[13],
                           instance.modelMatrix[14]);
    const float away = glm::distance(origin, camera);
    if (instance.lodDist > 0.f && away > instance.lodDist) return false;
    if (away < instance.childLodDist) return false;

    const Gta5Geometry* geometry = instance.geometry;
    if (!geometry || geometry->bounds[3] < 0.f) return true;
    glm::mat4 model(1.f);
    std::memcpy(glm::value_ptr(model), instance.modelMatrix.data(),
                sizeof(float) * 16);
    const glm::vec3 centre(model * glm::vec4(geometry->bounds[0],
                                             geometry->bounds[1],
                                             geometry->bounds[2], 1.f));
    const float scale = std::max({glm::length(glm::vec3(model[0])),
                                  glm::length(glm::vec3(model[1])),
                                  glm::length(glm::vec3(model[2]))});
    const float radius = geometry->bounds[3] * scale;
    if (radius < sizeRatio * glm::distance(centre, camera)) return false;
    for (const glm::vec4& plane : frustum.planes) {
        if (glm::dot(glm::vec3(plane), centre) + plane.w < -radius) {
            return false;
        }
    }
    return true;
}

}  // namespace sdl3cpp::services::impl
