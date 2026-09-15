#pragma once

#include <glm/glm.hpp>

namespace sdl3cpp::q3 {

/// Where the player is asking to go, and how fast.
struct Wish {
    glm::vec3 direction{0.0f};  ///< unit length, or zero when idle
    float speed{0.0f};
};

/**
 * @brief Turn movement input and a yaw into a wish direction and speed.
 *
 * The basis matches the camera's: forward is (-sin, 0, -cos) and right
 * is forward x up, so pressing forward goes where the player is looking.
 *
 * Speed follows ioq3's PM_CmdScale, which works out to
 * maxSpeed * max(|forward|, |right|). Using the vector's magnitude
 * instead makes a diagonal sqrt(2) times faster than a straight line,
 * which reads in game as the straight directions being reluctant.
 */
Wish ComputeWish(float moveForward, float moveRight, float yaw,
                 float maxSpeed);

}  // namespace sdl3cpp::q3
