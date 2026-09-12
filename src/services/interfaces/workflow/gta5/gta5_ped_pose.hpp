#pragma once

#include "services/interfaces/workflow/gta5/gta5_skeleton.hpp"

#include <vector>

namespace sdl3cpp::services::impl {

/// A ped's own axes, in the space its bones are posed in.
///
/// Nothing here is assumed. The values below are only what a GTA ped
/// ought to come out as; Gta5AxesOf measures all three off the bind
/// pose, because the space bones are posed in is not the space they
/// are drawn in, and getting up wrong stands a ped up with its arms
/// out rather than hanging them at its sides.
struct Gta5PedAxes {
    glm::vec3 up{0.f, 0.f, 1.f};
    glm::vec3 right{1.f, 0.f, 0.f};
    glm::vec3 front{0.f, 1.f, 0.f};
};

/// `s`'s axes, measured across its bind pose: one thigh to the other is
/// the way it is wide, and with up that settles the way it faces.
Gta5PedAxes Gta5AxesOf(const Gta5Skeleton& s);

/// The heading the bind pose already carries, counted as camera_yaw is
/// (0 looks along the engine's -z). A stance turns the ped from here,
/// so the body and the arms agree on which way is forward.
float Gta5PedBaseYaw(const Gta5PedAxes& axes);

/// A turn of `degrees` about `axis`, in the ped's own space. About its
/// right, positive swings a hanging limb forward.
glm::mat3 Gta5About(float degrees, const glm::vec3& axis);

/// Turn bone `name` by `turn`, given in the ped space at rest, about
/// its own joint: in the bone frame it stays a hinge as parents move.
void Gta5Turn(const Gta5Skeleton& s, std::vector<glm::mat4>& locals,
              const char* name, const glm::mat3& turn);

/// The bind pose holds the arms out in a T. This swings one from there
/// to point along `want`, in the ped space.
glm::mat3 Gta5ArmTowards(const Gta5Skeleton& s, const char* arm,
                         const char* fore, const glm::vec3& want);

/// Out from the body on whichever side this arm is on, as a unit.
glm::vec3 Gta5ArmSide(const Gta5Skeleton& s, const Gta5PedAxes& axes,
                      const char* arm, const char* fore);

/// Where an arm hangs with nothing in it: down the side of the body, a
/// hand's width out from it and a little forward, as one rests at ease.
glm::vec3 Gta5ArmRest(const Gta5Skeleton& s, const Gta5PedAxes& axes,
                      const char* arm, const char* fore);

/// Where an arm points held out down the sights, along a view pitched
/// `pitch` radians. `across` draws the hand in towards the centre line,
/// so the gun comes up under the eye rather than out at the shoulder.
glm::vec3 Gta5ArmAim(const Gta5Skeleton& s, const Gta5PedAxes& axes,
                     const char* arm, const char* fore, float pitch,
                     float across);

}  // namespace sdl3cpp::services::impl
