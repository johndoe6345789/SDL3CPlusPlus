#pragma once

#include "services/interfaces/workflow/fs2024/terrain/fs2024_heightfield.hpp"
#include "services/interfaces/workflow/quake3/pmove/q3_pm_types.hpp"

namespace sdl3cpp::services::impl {

/// Engine yaw (as camera_yaw and q3.pm use it) for a compass heading in
/// degrees: yaw 0 faces north (-z) and forward is (-sin yaw, -cos yaw).
float Fs2024YawForHeading(float headingDegrees);

/// The player origin that stands the box's feet `clearance` metres
/// above the ground at (x, z) on `field`.
glm::vec3 Fs2024StandingOrigin(const Fs2024Heightfield& field, float x,
                               float z, float clearance);

/// When the player has sunk below `field`'s ground, stand them back on
/// it with no downward speed. Returns true when it had to move them.
bool Fs2024KeepOnGround(const Fs2024Heightfield& field,
                        Q3PlayerState& player);

/// Move the physics body, if any, to `origin` and stop it.
void Fs2024MoveBody(btRigidBody* body, const glm::vec3& origin);

}  // namespace sdl3cpp::services::impl
