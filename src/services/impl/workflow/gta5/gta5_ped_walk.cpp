#include "services/interfaces/workflow/gta5/gta5_ped.hpp"
#include "services/interfaces/workflow/gta5/gta5_ped_arms.hpp"
#include "services/interfaces/workflow/gta5/gta5_ped_pose.hpp"

#include <algorithm>
#include <cmath>

namespace sdl3cpp::services::impl {
namespace {

constexpr float kPi = 3.14159265f;
constexpr float kStride = 1.4f;  // metres a two-step cycle covers

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
    // Hips, knees and shoulders all hinge on the way the ped is wide,
    // which its own bind pose is measured for rather than assumed.
    const Gta5PedAxes axes = Gta5AxesOf(s);
    const glm::vec3& hinge = axes.right;
    std::vector<glm::mat4> locals = s.locals;
    Gta5Turn(s, locals, "SKEL_L_Thigh", Gta5About(28.f * a * swing, hinge));
    Gta5Turn(s, locals, "SKEL_R_Thigh", Gta5About(-28.f * a * swing, hinge));
    // A knee bends as its leg swings through, and never forwards.
    Gta5Turn(s, locals, "SKEL_L_Calf",
             Gta5About(-45.f * a * std::max(0.f, lift), hinge));
    Gta5Turn(s, locals, "SKEL_R_Calf",
             Gta5About(-45.f * a * std::max(0.f, -lift), hinge));
    Gta5ArmSwing arms;
    arms.amount = a, arms.swing = swing, arms.aim = aim;
    // A pistol is fired one handed: the off hand stays where it hangs.
    arms.hold = twoHanded ? aim : 0.f, arms.pitch = pitch;
    PoseGta5PedArms(s, axes, arms, locals);
    std::vector<glm::mat4> pose;
    ComposeGta5Pose(s, locals, pose);
    skin.resize(pose.size());
    for (std::size_t i = 0; i < pose.size(); ++i) {
        skin[i] = pose[i] * s.unbind[i];
    }
}

}  // namespace sdl3cpp::services::impl
