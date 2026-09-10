#pragma once

#include <glm/glm.hpp>

namespace sdl3cpp::services::impl {

/// ioq3's default cloud height (tr_shader.c uses 512 when a sky shader's
/// skyParms omits it; q3dm1's tim_hell asks for 384).
inline constexpr float kDefaultCloudHeight = 512.0f;

/**
 * @brief Cloud-layer texture coordinates for a view direction.
 *
 * Ported from ioq3 tr_sky.c R_InitSkyTexCoords(): intersect the view ray
 * with a cloud sphere of radius radiusWorld + heightCloud centred one
 * radiusWorld below the viewer, then take the arc cosine of the two
 * horizontal components of the normalised hit point. Clouds stretch out
 * towards the horizon the way a high overcast does, rather than pinching
 * at the zenith as a plain spherical wrap would.
 *
 * @param direction Unit vector from the camera, in this engine's Y-up
 *                  space; Quake's Z-up convention is applied internally.
 */
glm::vec2 CloudTexCoords(const glm::vec3& direction, float heightCloud);

}  // namespace sdl3cpp::services::impl
