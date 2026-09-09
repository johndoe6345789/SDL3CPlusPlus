#pragma once

#include <string>
#include <vector>

namespace sdl3cpp::q3 {

/// One entry of a player model's animation.cfg.
struct AnimClip {
    int firstFrame{0};
    int numFrames{0};
    int loopingFrames{0};
    int fps{0};
};

/// Indices into the animation list, from ioq3 bg_public.h animNumber_t.
inline constexpr int kTorsoGesture = 6;
inline constexpr int kLegsWalkCr = 13;
inline constexpr int kTorsoGetFlag = 23;

/**
 * @brief Parse animation.cfg, applying ioq3's leg-frame adjustment.
 *
 * The file numbers frames across the whole character, but lower.md3
 * contains only the leg frames, so every LEGS_ entry starts too high by
 * the number of torso-only frames ahead of it. ioq3 subtracts that gap
 * in cg_players.c:
 *
 *   if ( i == LEGS_WALKCR )
 *       skip = animations[LEGS_WALKCR].firstFrame
 *            - animations[TORSO_GESTURE].firstFrame;
 *   if ( i >= LEGS_WALKCR && i < TORSO_GETFLAG )
 *       animations[i].firstFrame -= skip;
 *
 * Without it the leg indices run off the end of the model and the legs
 * animate from whatever happens to be there.
 */
std::vector<AnimClip> ParseAnimCfg(const std::string& text);

}  // namespace sdl3cpp::q3
