#pragma once

#include <glm/glm.hpp>

namespace sdl3cpp::q3 {

/// Overbounce factor Quake uses; slightly over 1 so the player does not
/// stick to the surface they just clipped against.
inline constexpr float kOverclip = 1.001f;

/// A dot product at or above this counts as "not entering" the plane.
/// Quake uses a tolerance rather than zero so floating point noise on a
/// surface being slid along does not read as a fresh collision.
inline constexpr float kIntoEpsilon = 0.1f;

/// Planes closer than this in orientation are treated as the same plane.
inline constexpr float kSamePlaneDot = 0.99f;

/// v with its component along `normal` removed, scaled by `overbounce`.
glm::vec3 ClipVelocity(const glm::vec3& velocity, const glm::vec3& normal,
                       float overbounce);

/**
 * @brief Make a velocity parallel to every clip plane it is entering.
 *
 * Follows ioq3 bg_slidemove.c PM_SlideMove: clip against the first plane
 * entered, and if that pushes into a second, clip again and fall back to
 * the crease those two form. Velocity is only killed outright when a
 * third plane is still entered, which is a genuine corner.
 *
 * @return the resolved velocity, or a zero vector on a triple-plane
 *         interaction.
 */
glm::vec3 ResolveAgainstPlanes(const glm::vec3& velocity,
                               const glm::vec3* planes, int numPlanes);

}  // namespace sdl3cpp::q3
