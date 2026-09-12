#include "services/interfaces/workflow/gta5/weapon/gta5_weapon_ray.hpp"

namespace sdl3cpp::services::impl {

glm::vec3 Gta5MuzzlePoint(const glm::vec3& eye, const glm::vec3& ahead,
                          const glm::vec3& origin, float head,
                          bool thirdPerson) {
    // Looking down the sights, the gun is just ahead of the eye.
    if (!thirdPerson) return eye + ahead * 0.35f;
    const glm::vec3 up(0.f, 1.f, 0.f);
    glm::vec3 side = glm::cross(ahead, up);
    const float much = glm::length(side);
    side = much > 0.001f ? side / much : glm::vec3(1.f, 0.f, 0.f);
    return origin + up * (head * 0.45f) + ahead * 0.45f + side * 0.2f;
}

}  // namespace sdl3cpp::services::impl
