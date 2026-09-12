#include "services/interfaces/workflow/gta5/gta5_ped.hpp"
#include "services/interfaces/workflow/gta5/gta5_ped_arms.hpp"
#include "services/interfaces/workflow/gta5/gta5_ped_gait.hpp"

#include <algorithm>
#include <cmath>
#include <cmath>

namespace sdl3cpp::services::impl {
namespace {

constexpr float kPi = 3.14159265f;

}  // namespace

void PoseGta5Ped(const Gta5Skeleton& s, Gta5PedWalk& walk, float speed,
                 float dt, float aim, float pitch, bool twoHanded,
                 std::vector<glm::mat4>& skin) {
    const Gta5Gait gait = Gta5GaitFor(speed);
    // Paced by the ground it covers, so the feet keep up with the map
    // rather than running on the spot; capped, as q3 can outrun a man.
    const float rate = std::min(speed, 9.f) / gait.stride * 2.f * kPi;
    walk.phase = std::fmod(walk.phase + rate * dt, 2.f * kPi);
    walk.breath = std::fmod(walk.breath + dt, 2.f * kPi * 100.f);
    // Eased in and out so a step does not snap on the moment he moves.
    const float wanted = std::clamp(speed / 1.2f, 0.f, 1.f);
    walk.amount += (wanted - walk.amount) * std::min(1.f, dt * 9.f);
    const float a = walk.amount;
    // Standing still he breathes rather than freezing mid-stride.
    const float breath = (1.f - a) * std::sin(walk.breath * 1.6f);
    const Gta5PedAxes axes = Gta5AxesOf(s);
    std::vector<glm::mat4> locals = s.locals;
    PoseGta5PedLegs(s, axes, gait, walk.phase, a, locals);
    PoseGta5PedTorso(s, axes, gait, walk.phase, a, breath, locals);
    Gta5ArmSwing arms;
    arms.amount = a, arms.swing = std::sin(walk.phase), arms.aim = aim;
    // A pistol is fired one handed: the off hand stays where it hangs.
    arms.hold = twoHanded ? aim : 0.f, arms.pitch = pitch;
    arms.degrees = gait.arm, arms.elbow = gait.elbow;
    PoseGta5PedArms(s, axes, arms, locals);
    std::vector<glm::mat4> pose;
    ComposeGta5Pose(s, locals, pose);
    // Back onto his feet. A knee bent under the stance shortens the leg,
    // and a run lifts him further still; left alone he walks above the
    // road, and putting the shoe back down is what the bob really is.
    const float floating = Gta5FootFloat(s, axes, pose);
    if (std::fabs(floating) > 1e-4f) {
        Gta5Shift(s, locals, "SKEL_ROOT", axes.up * -floating);
        ComposeGta5Pose(s, locals, pose);
    }
    skin.resize(pose.size());
    for (std::size_t i = 0; i < pose.size(); ++i) {
        skin[i] = pose[i] * s.unbind[i];
    }
}

}  // namespace sdl3cpp::services::impl
