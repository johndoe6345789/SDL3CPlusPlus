#include "services/interfaces/workflow/quake3/q3_bot_usercmd.hpp"

#include "services/interfaces/workflow/quake3/q3_pm_constants.hpp"

#include <algorithm>
#include <cmath>

namespace sdl3cpp::q3 {

Q3UserCmd BotDirectionToUserCmd(const glm::vec3& worldDir, float speedFraction,
                                float yaw) {
    Q3UserCmd cmd;
    cmd.yaw = yaw;

    // The basis ComputeWish reads the command back through, so that a
    // command built here reproduces the direction it was built from.
    const glm::vec3 forward(-std::sin(yaw), 0.0f, -std::cos(yaw));
    const glm::vec3 right(std::cos(yaw), 0.0f, -std::sin(yaw));

    const glm::vec3 flat(worldDir.x, 0.0f, worldDir.z);
    const float length = glm::length(flat);
    if (length < 0.0001f || speedFraction <= 0.0f) {
        return cmd;
    }
    const glm::vec3 dir = flat / length;

    float f = glm::dot(forward, dir);
    float r = glm::dot(right, dir);

    const float m = std::max(std::fabs(f), std::fabs(r));
    if (m > 0.0f) {
        f *= speedFraction / m;
        r *= speedFraction / m;
    }

    cmd.forwardMove = f;
    cmd.rightMove   = r;
    return cmd;
}

float BotSpeedFraction(float moveSpeed) {
    if (moveSpeed <= 0.0f) return 0.0f;
    return std::min(1.0f, moveSpeed / kMaxSpeed);
}

}  // namespace sdl3cpp::q3
