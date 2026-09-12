#include "services/interfaces/workflow/gta5/ped/gta5_ped_pose.hpp"

#include <cmath>

namespace sdl3cpp::services::impl {

glm::vec3 Gta5ArmSide(const Gta5Skeleton& s, const Gta5PedAxes& axes,
                      const char* arm, const char* fore) {
    const int a = FindGta5Bone(s, arm), f = FindGta5Bone(s, fore);
    if (a < 0 || f < 0) return axes.right;
    // The bind pose holds the arms out in a T, so the way one runs is
    // the side of the body it hangs on.
    const glm::vec3 along(s.rest[f][3] - s.rest[a][3]);
    return glm::dot(along, axes.right) > 0.f ? axes.right : -axes.right;
}

glm::vec3 Gta5ArmRest(const Gta5Skeleton& s, const Gta5PedAxes& axes,
                      const char* arm, const char* fore) {
    // Down, not back: an arm at ease hangs along the body, just clear
    // of the hip and a little ahead of it.
    return glm::normalize(-axes.up + axes.front * 0.1f +
                          Gta5ArmSide(s, axes, arm, fore) * 0.16f);
}

glm::vec3 Gta5ArmAim(const Gta5Skeleton& s, const Gta5PedAxes& axes,
                     const char* arm, const char* fore, float pitch,
                     float across) {
    const glm::vec3 sights =
        axes.front * std::cos(pitch) + axes.up * std::sin(pitch);
    return glm::normalize(sights -
                          Gta5ArmSide(s, axes, arm, fore) * across);
}

}  // namespace sdl3cpp::services::impl
