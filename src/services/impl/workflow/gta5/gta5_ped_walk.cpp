#include "services/interfaces/workflow/gta5/gta5_ped.hpp"

#include <glm/gtc/matrix_transform.hpp>

#include <algorithm>
#include <cmath>

namespace sdl3cpp::services::impl {
namespace {

constexpr float kPi = 3.14159265f;
constexpr float kStride = 1.4f;  // metres a two-step cycle covers

/// A turn of `degrees` about `axis`, in the ped's own space: it faces +y,
/// +x is its right, +z up.
glm::mat3 About(float degrees, glm::vec3 axis = glm::vec3(1.f, 0.f, 0.f)) {
    return glm::mat3(glm::rotate(glm::mat4(1.f), glm::radians(degrees), axis));
}

/// Turn bone `name` by `turn`, given in the ped's space at rest, about its
/// own joint. In the bone's frame it stays a hinge as parents move.
void Turn(const Gta5Skeleton& s, std::vector<glm::mat4>& locals,
          const char* name, const glm::mat3& turn) {
    const int b = FindGta5Bone(s, name);
    if (b < 0) return;
    const glm::mat3 frame(s.rest[b]);
    locals[b] = locals[b] * glm::mat4(glm::inverse(frame) * turn * frame);
}

/// The bind pose holds the arms out in a T: this lowers one to hang a
/// little away from the body.
glm::mat3 ArmDown(const Gta5Skeleton& s, const char* arm, const char* fore) {
    const int a = FindGta5Bone(s, arm), f = FindGta5Bone(s, fore);
    if (a < 0 || f < 0) return glm::mat3(1.f);
    const glm::vec3 along =
        glm::normalize(glm::vec3(s.rest[f][3] - s.rest[a][3]));
    const glm::vec3 down =
        glm::normalize(glm::vec3(along.x > 0.f ? 0.2f : -0.2f, 0.f, -1.f));
    const glm::vec3 axis = glm::cross(along, down);
    if (glm::length(axis) < 1e-4f) return glm::mat3(1.f);
    const float angle = std::acos(std::clamp(glm::dot(along, down), -1.f, 1.f));
    return glm::mat3(glm::rotate(glm::mat4(1.f), angle, glm::normalize(axis)));
}

}  // namespace

void PoseGta5Ped(const Gta5Skeleton& s, Gta5PedWalk& walk, float speed,
                 float dt, std::vector<glm::mat4>& skin) {
    // Paced by distance so the feet keep up with the ground; capped, as q3
    // runs at twice a sprint.
    const float rate = std::min(speed, 6.f) / kStride * 2.f * kPi;
    walk.phase = std::fmod(walk.phase + rate * dt, 2.f * kPi);
    const float wanted = std::clamp(speed / 3.f, 0.f, 1.f);
    walk.amount += (wanted - walk.amount) * std::min(1.f, dt * 8.f);
    const float a = walk.amount, swing = std::sin(walk.phase);
    const float lift = std::cos(walk.phase);
    std::vector<glm::mat4> locals = s.locals;
    Turn(s, locals, "SKEL_L_Thigh", About(28.f * a * swing));
    Turn(s, locals, "SKEL_R_Thigh", About(-28.f * a * swing));
    // A knee bends as its leg swings through, and never backwards.
    Turn(s, locals, "SKEL_L_Calf", About(-45.f * a * std::max(0.f, lift)));
    Turn(s, locals, "SKEL_R_Calf", About(-45.f * a * std::max(0.f, -lift)));
    // Arms hang, swing against the legs, and bend a little at the elbow
    // -- about the hinge the lowered arm has now, not the T-pose's.
    const glm::mat3 left = ArmDown(s, "SKEL_L_UpperArm", "SKEL_L_Forearm");
    const glm::mat3 right = ArmDown(s, "SKEL_R_UpperArm", "SKEL_R_Forearm");
    Turn(s, locals, "SKEL_L_UpperArm", About(-20.f * a * swing) * left);
    Turn(s, locals, "SKEL_R_UpperArm", About(20.f * a * swing) * right);
    const glm::mat3 elbow = About(12.f + 18.f * a);
    Turn(s, locals, "SKEL_L_Forearm", glm::inverse(left) * elbow * left);
    Turn(s, locals, "SKEL_R_Forearm", glm::inverse(right) * elbow * right);
    std::vector<glm::mat4> pose;
    ComposeGta5Pose(s, locals, pose);
    skin.resize(pose.size());
    for (std::size_t i = 0; i < pose.size(); ++i) {
        skin[i] = pose[i] * s.unbind[i];
    }
}

}  // namespace sdl3cpp::services::impl
