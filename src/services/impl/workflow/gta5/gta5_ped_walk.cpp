#include "services/interfaces/workflow/gta5/gta5_ped.hpp"
#include "services/interfaces/workflow/gta5/gta5_ped_pose.hpp"

#include <algorithm>
#include <cmath>

namespace sdl3cpp::services::impl {
namespace {

constexpr float kPi = 3.14159265f;
constexpr float kStride = 1.4f;  // metres a two-step cycle covers

/// Where an arm points: hanging with nothing held, down the sights as
/// the weapon comes up. The ped looks along its own +z.
glm::vec3 Wants(const Gta5Skeleton& s, const char* arm, const char* fore,
                float aim, float pitch, float across) {
    const glm::vec3 rest = Gta5ArmRest(s, arm, fore);
    if (aim <= 0.001f) return rest;
    const glm::vec3 sights(across, std::sin(pitch), std::cos(pitch));
    return glm::normalize(
        glm::mix(rest, glm::normalize(sights), std::min(aim, 1.f)));
}

}  // namespace

void PoseGta5Ped(const Gta5Skeleton& s, Gta5PedWalk& walk, float speed,
                 float dt, float aim, float pitch, bool twoHanded,
                 std::vector<glm::mat4>& skin) {
    // Paced by distance so the feet keep up with the ground; capped, as
    // q3 runs at twice a sprint.
    const float rate = std::min(speed, 6.f) / kStride * 2.f * kPi;
    walk.phase = std::fmod(walk.phase + rate * dt, 2.f * kPi);
    const float wanted = std::clamp(speed / 3.f, 0.f, 1.f);
    walk.amount += (wanted - walk.amount) * std::min(1.f, dt * 8.f);
    const float a = walk.amount, swing = std::sin(walk.phase);
    const float lift = std::cos(walk.phase);
    std::vector<glm::mat4> locals = s.locals;
    Gta5Turn(s, locals, "SKEL_L_Thigh", Gta5About(28.f * a * swing));
    Gta5Turn(s, locals, "SKEL_R_Thigh", Gta5About(-28.f * a * swing));
    // A knee bends as its leg swings through, and never backwards.
    Gta5Turn(s, locals, "SKEL_L_Calf",
             Gta5About(-45.f * a * std::max(0.f, lift)));
    Gta5Turn(s, locals, "SKEL_R_Calf",
             Gta5About(-45.f * a * std::max(0.f, -lift)));
    // The shooting arm comes up level with the sights and the other
    // holds the fore end; both still swing while the hands are down.
    const float rest = 1.f - std::min(aim, 1.f);
    // A pistol is fired one handed: the off hand stays where it hangs.
    const float hold = twoHanded ? aim : 0.f;
    const float restL = 1.f - std::min(hold, 1.f);
    const glm::mat3 left = Gta5ArmTowards(
        s, "SKEL_L_UpperArm", "SKEL_L_Forearm",
        Wants(s, "SKEL_L_UpperArm", "SKEL_L_Forearm", hold, pitch, 0.18f));
    const glm::mat3 right = Gta5ArmTowards(
        s, "SKEL_R_UpperArm", "SKEL_R_Forearm",
        Wants(s, "SKEL_R_UpperArm", "SKEL_R_Forearm", aim, pitch, -0.12f));
    Gta5Turn(s, locals, "SKEL_L_UpperArm",
             Gta5About(-20.f * a * swing * restL) * left);
    Gta5Turn(s, locals, "SKEL_R_UpperArm",
             Gta5About(20.f * a * swing * rest) * right);
    // The elbows straighten as the gun comes up.
    const glm::mat3 elbow = Gta5About((12.f + 18.f * a) * rest);
    const glm::mat3 elbowL = Gta5About((12.f + 18.f * a) * restL);
    Gta5Turn(s, locals, "SKEL_L_Forearm",
             glm::inverse(left) * elbowL * left);
    Gta5Turn(s, locals, "SKEL_R_Forearm",
             glm::inverse(right) * elbow * right);
    std::vector<glm::mat4> pose;
    ComposeGta5Pose(s, locals, pose);
    skin.resize(pose.size());
    for (std::size_t i = 0; i < pose.size(); ++i) {
        skin[i] = pose[i] * s.unbind[i];
    }
}

}  // namespace sdl3cpp::services::impl
