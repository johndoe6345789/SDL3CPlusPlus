#pragma once

#include <glm/glm.hpp>

#include <cstdint>

namespace sdl3cpp::services::impl {

/// Q3A-style idle head sway state (mirrors cg.headStart/EndYaw/Pitch/Time).
struct HeadSwayState {
    float swayStartYaw   = glm::radians(180.f);
    float swayStartPitch = 0.f;
    float swayEndYaw     = glm::radians(180.f);
    float swayEndPitch   = 0.f;
    uint64_t swayStartMs = 0;
    uint64_t swayEndMs   = 0;
};

/// One yaw/pitch pair, in radians.
struct HeadAngles {
    float yaw   = 0.f;
    float pitch = 0.f;
};

/**
 * @brief Advances the idle head-sway animation and returns the current pose.
 *
 * Mirrors ioq3 CG_DrawStatusBarHead: every 100-2100 ms picks a new target
 * yaw (180 deg +/- 20 deg) and pitch (+/- 5 deg), smoothstep-interpolating
 * toward it. Mutates `state` in place.
 */
HeadAngles UpdateHeadSway(HeadSwayState& state, uint64_t nowMs);

/**
 * @brief Builds the head-at-origin MVP for the given sway pose.
 *
 * The camera orbits the head at `camDist` using `angles`; Q3A places its
 * origin at (len/tan(15 deg), 0, 0) in model space, which this
 * approximates. Uses a 30 deg FOV to match Q3A's status-bar head.
 */
glm::mat4 BuildHeadPortraitMvp(const HeadAngles& angles, float camDist);

}  // namespace sdl3cpp::services::impl
