#include "services/interfaces/workflow/gta5/ped/gta5_ped_pose.hpp"

#include <cmath>

namespace sdl3cpp::services::impl {
namespace {

/// Where a bone stands in the bind pose. False when there is no such
/// bone, and `at` is left alone.
bool Where(const Gta5Skeleton& s, const char* name, glm::vec3& at) {
    const int b = FindGta5Bone(s, name);
    if (b < 0 || static_cast<std::size_t>(b) >= s.rest.size()) return false;
    at = glm::vec3(s.rest[b][3]);
    return true;
}

}  // namespace

Gta5PedAxes Gta5AxesOf(const Gta5Skeleton& s) {
    Gta5PedAxes axes;
    glm::vec3 head(0.f), left(0.f), right(0.f);
    if (!Where(s, "SKEL_Head", head)) return axes;
    if (!Where(s, "SKEL_L_Thigh", left)) return axes;
    if (!Where(s, "SKEL_R_Thigh", right)) return axes;
    // Nor is up assumed: a ped stands, so its head over the middle of
    // its hips is which way is up in whatever space the bones were
    // read in. Both hips, not one: either alone leans it sideways.
    const glm::vec3 hips = (left + right) * 0.5f;
    if (glm::length(head - hips) < 0.2f) return axes;
    axes.up = glm::normalize(head - hips);
    // Hip to hip, flat on the ground: a ped is widest across itself,
    // and which way that runs says which way the rest of it faces.
    glm::vec3 across = right - left;
    across -= axes.up * glm::dot(axes.up, across);
    if (glm::length(across) < 1e-4f) return axes;
    axes.right = glm::normalize(across);
    axes.front = glm::cross(axes.up, axes.right);
    return axes;
}

float Gta5PedBaseYaw(const Gta5PedAxes& axes) {
    // The engine's space is (x, z, -y) of this one, so a bind pose
    // facing `front` looks along (front.x, 0, -front.y) over there.
    // camera_yaw counts from -z and grows to the left.
    return std::atan2(-axes.front.x, axes.front.y);
}

}  // namespace sdl3cpp::services::impl
