#pragma once

#include "services/interfaces/workflow/gta5/gta5_ped_pose.hpp"

#include <vector>

namespace sdl3cpp::services::impl {

/// How far through a walk the arms are, and how far up the gun is.
struct Gta5ArmSwing {
    float amount{0.f};  // 0 standing, 1 walking
    float swing{0.f};   // sine of the walk's phase
    float aim{0.f};     // the gun hand: 0 hanging, 1 down the sights
    float hold{0.f};    // the off hand: only a two handed gun takes it
    float pitch{0.f};   // where the view looks, radians, + is up
};

/// Both arms into `locals`. The gun hand comes up down the sights as
/// `aim` goes to one and the off hand only joins it on a weapon that
/// wants two; whatever is left hanging swings with the walk and keeps
/// its elbow bent, so a pistol is fired one armed with the other arm
/// still swinging at the side.
void PoseGta5PedArms(const Gta5Skeleton& s, const Gta5PedAxes& axes,
                     const Gta5ArmSwing& walk,
                     std::vector<glm::mat4>& locals);

}  // namespace sdl3cpp::services::impl
