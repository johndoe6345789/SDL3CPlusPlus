#include "services/interfaces/workflow/gta5/gta5_reflection.hpp"

#include <glm/gtc/matrix_transform.hpp>

#include <algorithm>

namespace sdl3cpp::services::impl {

float ChooseGta5MirrorHeight(const std::vector<Gta5WaterQuad>& water,
                             const glm::vec3& eye) {
    // Engine x is GTA's x, and engine z its -y.
    const float x = eye.x, y = -eye.z;
    float best = -1.f, height = 0.f;
    for (const Gta5WaterQuad& q : water) {
        if (q.z > eye.y + 1.f) continue;  // above the eye: not looked down on
        const float dx = std::max({q.minX - x, 0.f, x - q.maxX});
        const float dy = std::max({q.minY - y, 0.f, y - q.maxY});
        const float d = dx * dx + dy * dy;
        if (best < 0.f || d < best || (d == best && q.z > height)) {
            best = d;
            height = q.z;
        }
    }
    return height;
}

glm::mat4 Gta5MirrorAt(float height) {
    return glm::translate(glm::mat4(1.f), glm::vec3(0.f, height, 0.f)) *
           glm::scale(glm::mat4(1.f), glm::vec3(1.f, -1.f, 1.f)) *
           glm::translate(glm::mat4(1.f), glm::vec3(0.f, -height, 0.f));
}

glm::mat4 Gta5ObliqueProjection(const glm::mat4& proj,
                                const glm::vec4& plane) {
    const glm::mat4 inverse = glm::inverse(proj);
    // The far corner of the frustum on the kept side, back in view space.
    const glm::vec4 clip = glm::transpose(inverse) * plane;
    const glm::vec4 corner =
        inverse * glm::vec4(glm::sign(clip.x), glm::sign(clip.y), 1.f, 1.f);
    const glm::vec4 c = plane * (1.f / glm::dot(plane, corner));
    glm::mat4 out = proj;
    // The row that makes clip z: depth 0 now lies on the plane.
    out[0][2] = c.x, out[1][2] = c.y, out[2][2] = c.z, out[3][2] = c.w;
    return out;
}

}  // namespace sdl3cpp::services::impl
