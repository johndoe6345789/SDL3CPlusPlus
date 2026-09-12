#include "services/interfaces/workflow/gta5/gta5_held_weapon.hpp"

#include <glm/gtc/matrix_transform.hpp>

namespace sdl3cpp::services::impl {

bool Gta5HandMatrix(const Gta5Skeleton& skeleton,
                    const std::vector<glm::mat4>& skin, glm::mat4& hand) {
    const int b = FindGta5Bone(skeleton, "SKEL_R_Hand");
    if (b < 0 || static_cast<std::size_t>(b) >= skin.size()) return false;
    if (static_cast<std::size_t>(b) >= skeleton.rest.size()) return false;
    // Through its rest pose: where the bone has ended up.
    hand = skin[b] * skeleton.rest[b];
    return true;
}

glm::mat4 Gta5GripTurn(const Gta5Skeleton& s) {
    const int h = FindGta5Bone(s, "SKEL_R_Hand");
    const int f = FindGta5Bone(s, "SKEL_R_Forearm");
    if (h < 0 || f < 0) return glm::mat4(1.f);
    if (static_cast<std::size_t>(h) >= s.rest.size()) return glm::mat4(1.f);
    if (static_cast<std::size_t>(f) >= s.rest.size()) return glm::mat4(1.f);
    // A gun is gripped, so its barrel lies along the way the hand
    // points and its top follows the back of the hand. Both are taken
    // in the bone own frame, which is why this holds however the arm
    // is posed: hanging, the barrel points at the floor; brought up to
    // aim, it points where the hand does. The barrel is the model x,
    // measured: a weapon .ydr is far longer along it than across.
    const glm::mat3 frame(s.rest[h]);
    const glm::mat3 inv = glm::inverse(frame);
    const glm::vec3 reach(s.rest[h][3] - s.rest[f][3]);
    glm::vec3 along = inv * reach;
    if (glm::length(along) < 1e-5f) return glm::mat4(1.f);
    along = glm::normalize(along);
    glm::vec3 up = inv * glm::vec3(0.f, 1.f, 0.f);
    up = up - along * glm::dot(along, up);
    if (glm::length(up) < 1e-5f) return glm::mat4(1.f);
    up = glm::normalize(up);
    return glm::mat4(glm::mat3(along, up, glm::cross(along, up)));
}

}  // namespace sdl3cpp::services::impl
