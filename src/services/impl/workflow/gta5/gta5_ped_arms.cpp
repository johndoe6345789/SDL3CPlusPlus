#include "services/interfaces/workflow/gta5/gta5_ped_arms.hpp"

#include <algorithm>
#include <cmath>

namespace sdl3cpp::services::impl {
namespace {

/// One direction swung round to another along the shortest arc. Mixing
/// the two straight would cut the corner, dropping the hand in towards
/// the hip half way up; on the arc it sweeps, at an even rate.
glm::vec3 Sweep(const glm::vec3& from, const glm::vec3& to, float t) {
    const float much = std::clamp(glm::dot(from, to), -1.f, 1.f);
    const float angle = std::acos(much);
    const glm::vec3 out = to - from * much;
    if (angle < 1e-3f || glm::length(out) < 1e-5f) return to;
    return from * std::cos(angle * t) +
           glm::normalize(out) * std::sin(angle * t);
}

/// Where an arm points: hanging at the side with nothing held, out
/// down the sights as it comes up, and part way round while it rises.
glm::vec3 Wants(const Gta5Skeleton& s, const Gta5PedAxes& axes,
                const char* arm, const char* fore, float up, float pitch,
                float across) {
    const glm::vec3 rest = Gta5ArmRest(s, axes, arm, fore);
    if (up <= 0.001f) return rest;
    const glm::vec3 aim = Gta5ArmAim(s, axes, arm, fore, pitch, across);
    return Sweep(rest, aim, std::min(up, 1.f));
}

/// One arm swung to `want`, swaying with the walk and bending its elbow
/// by `bend` degrees -- both only by however much of it still hangs.
void Arm(const Gta5Skeleton& s, const glm::vec3& hinge,
         std::vector<glm::mat4>& locals, const char* arm, const char* fore,
         const glm::vec3& want, float sway, float bend) {
    const glm::mat3 turn = Gta5ArmTowards(s, arm, fore, want);
    Gta5Turn(s, locals, arm, Gta5About(sway, hinge) * turn);
    // In the shoulder's turned frame, so the elbow stays a hinge.
    Gta5Turn(s, locals, fore,
             glm::inverse(turn) * Gta5About(bend, hinge) * turn);
}

}  // namespace

void PoseGta5PedArms(const Gta5Skeleton& s, const Gta5PedAxes& axes,
                     const Gta5ArmSwing& walk,
                     std::vector<glm::mat4>& locals) {
    const float aim = std::min(walk.aim, 1.f);
    const float hold = std::min(walk.hold, 1.f);
    const float a = walk.amount, swing = walk.swing;
    const float downL = 1.f - hold, downR = 1.f - aim;
    // The off hand reaches further across: it takes the fore end, in
    // front of the hand that holds the grip.
    const glm::vec3 wantL = Wants(s, axes, "SKEL_L_UpperArm",
                                  "SKEL_L_Forearm", hold, walk.pitch, 0.22f);
    const glm::vec3 wantR = Wants(s, axes, "SKEL_R_UpperArm",
                                  "SKEL_R_Forearm", aim, walk.pitch, 0.12f);
    Arm(s, axes.right, locals, "SKEL_L_UpperArm", "SKEL_L_Forearm", wantL,
        -20.f * a * swing * downL, (12.f + 18.f * a) * downL);
    Arm(s, axes.right, locals, "SKEL_R_UpperArm", "SKEL_R_Forearm", wantR,
        20.f * a * swing * downR, (12.f + 18.f * a) * downR);
}

}  // namespace sdl3cpp::services::impl
