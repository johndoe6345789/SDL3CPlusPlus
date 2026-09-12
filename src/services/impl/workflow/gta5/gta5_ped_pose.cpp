#include "services/interfaces/workflow/gta5/gta5_ped_pose.hpp"

#include <glm/gtc/matrix_transform.hpp>

#include <algorithm>
#include <cmath>

namespace sdl3cpp::services::impl {

glm::mat3 Gta5About(float degrees, const glm::vec3& axis) {
    return glm::mat3(
        glm::rotate(glm::mat4(1.f), glm::radians(degrees), axis));
}

void Gta5Turn(const Gta5Skeleton& s, std::vector<glm::mat4>& locals,
              const char* name, const glm::mat3& turn) {
    const int b = FindGta5Bone(s, name);
    if (b < 0 || static_cast<std::size_t>(b) >= locals.size()) return;
    const glm::mat3 frame(s.rest[b]);
    locals[b] = locals[b] * glm::mat4(glm::inverse(frame) * turn * frame);
}

glm::mat3 Gta5ArmTowards(const Gta5Skeleton& s, const char* arm,
                         const char* fore, const glm::vec3& want) {
    const int a = FindGta5Bone(s, arm), f = FindGta5Bone(s, fore);
    if (a < 0 || f < 0) return glm::mat3(1.f);
    const glm::vec3 along =
        glm::normalize(glm::vec3(s.rest[f][3] - s.rest[a][3]));
    const glm::vec3 to = glm::normalize(want);
    const glm::vec3 axis = glm::cross(along, to);
    if (glm::length(axis) < 1e-4f) return glm::mat3(1.f);
    const float angle =
        std::acos(std::clamp(glm::dot(along, to), -1.f, 1.f));
    return glm::mat3(
        glm::rotate(glm::mat4(1.f), angle, glm::normalize(axis)));
}

}  // namespace sdl3cpp::services::impl
