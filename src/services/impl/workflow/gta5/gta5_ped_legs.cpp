#include "services/interfaces/workflow/gta5/gta5_ped_gait.hpp"

#include <algorithm>
#include <cmath>

namespace sdl3cpp::services::impl {
namespace {

constexpr float kPi = 3.14159265f;

/// One leg through the cycle. Hip, knee and ankle all hinge on the same
/// axis, so their angles simply add: the foot's own pitch is the sum of
/// the three, which is what lets the sole be held flat to the ground.
void Leg(const Gta5Skeleton& s, const glm::vec3& hinge, const Gta5Gait& g,
         float phase, float amount, const char* thigh, const char* calf,
         const char* foot, std::vector<glm::mat4>& locals) {
    const float swing = std::sin(phase), lift = std::cos(phase);
    const float stance = std::max(0.f, -lift);  // 1 with the foot down
    const float hip = g.thigh * amount * swing;
    // Folded most through mid swing, with a softer dip under the weight
    // of the stance that keeps the standing leg off a locked knee.
    const float knee = -g.knee * amount *
                       (std::max(0.f, lift) + 0.16f * stance);
    // The sole holds the ground it stands on, then rolls off the toe as
    // the leg goes behind; in the air it hangs most of the way back.
    const float flat = 0.35f + 0.65f * stance;
    const float roll = g.ankle * amount * std::max(0.f, -swing) * stance;
    Gta5Turn(s, locals, thigh, Gta5About(hip, hinge));
    Gta5Turn(s, locals, calf, Gta5About(knee, hinge));
    Gta5Turn(s, locals, foot, Gta5About(-(hip + knee) * flat - roll, hinge));
}

}  // namespace

void PoseGta5PedLegs(const Gta5Skeleton& s, const Gta5PedAxes& axes,
                     const Gta5Gait& gait, float phase, float amount,
                     std::vector<glm::mat4>& locals) {
    Leg(s, axes.right, gait, phase, amount, "SKEL_L_Thigh", "SKEL_L_Calf",
        "SKEL_L_Foot", locals);
    // Half a cycle apart: one foot lands as the other leaves.
    Leg(s, axes.right, gait, phase + kPi, amount, "SKEL_R_Thigh",
        "SKEL_R_Calf", "SKEL_R_Foot", locals);
}

}  // namespace sdl3cpp::services::impl
