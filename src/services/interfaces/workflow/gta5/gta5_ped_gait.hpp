#pragma once

#include "services/interfaces/workflow/gta5/gta5_ped_pose.hpp"

namespace sdl3cpp::services::impl {

/// How a ped carries itself at one speed.
///
/// A walk and a run are not the same motion played at two rates: the
/// stride lengthens, the knee folds further, the chest comes forward
/// over the hips and the arms stop hanging and start driving. Every
/// number here differs between the two, and a jog is the mix.
struct Gta5Gait {
    float stride{1.35f};  // metres a two-step cycle covers
    float thigh{22.f};    // degrees the hip swings, fore and aft
    float knee{55.f};     // degrees the knee folds through the swing
    float ankle{18.f};    // degrees the foot rolls off its toe
    float arm{16.f};      // degrees the shoulder swings
    float elbow{20.f};    // degrees the elbow holds
    float lean{2.f};      // degrees the chest sits ahead of the hips
    float twist{6.f};     // degrees the hips turn, the chest opposing
    float sway{0.035f};   // metres it leans onto the standing leg
};

/// The gait at `speed` m/s: a walk, a run, or the mix between them.
Gta5Gait Gta5GaitFor(float speed);

/// The legs, feet and all, `phase` radians through the cycle. `amount`
/// fades the whole of it out as the ped comes to a stand.
void PoseGta5PedLegs(const Gta5Skeleton& s, const Gta5PedAxes& axes,
                     const Gta5Gait& gait, float phase, float amount,
                     std::vector<glm::mat4>& locals);

/// How far the posed feet float above where the bind pose stood: what
/// has to come off the body to put the shoe back on the road.
float Gta5FootFloat(const Gta5Skeleton& s, const Gta5PedAxes& axes,
                    const std::vector<glm::mat4>& pose);

/// The hips, back and head: the body rising and leaning over whichever
/// foot is down, the hips turning with the stride and the chest turning
/// against them. `breath` is the slow rise and fall of standing still.
void PoseGta5PedTorso(const Gta5Skeleton& s, const Gta5PedAxes& axes,
                      const Gta5Gait& gait, float phase, float amount,
                      float breath, std::vector<glm::mat4>& locals);

}  // namespace sdl3cpp::services::impl
