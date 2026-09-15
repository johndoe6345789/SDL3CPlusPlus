#pragma once

namespace sdl3cpp::q3 {

/// ioq3 chops a move longer than this into pieces so behaviour does not
/// become framerate dependent (bg_pmove.c Pmove: "msec > 66").
inline constexpr float kMaxFrameSeconds = 0.066f;

/// A frame this short is treated as a stall rather than real elapsed
/// time; below it the physics gains nothing and denormals creep in.
inline constexpr float kMinFrameSeconds = 0.0005f;

/**
 * @brief Clamp a measured frame time to the range pmove can integrate.
 *
 * A step reading a fixed default instead of the real elapsed time moves
 * the player by that fixed amount every frame, so the faster the game
 * runs the faster the player travels. Clamping keeps a long stall from
 * teleporting the player through geometry in one step.
 */
constexpr float ClampFrameSeconds(float seconds) {
    if (!(seconds > kMinFrameSeconds)) {  // also catches NaN
        return kMinFrameSeconds;
    }
    return seconds > kMaxFrameSeconds ? kMaxFrameSeconds : seconds;
}

}  // namespace sdl3cpp::q3
