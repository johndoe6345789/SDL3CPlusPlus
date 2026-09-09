#include "services/interfaces/workflow/quake3/q3_wish_dir.hpp"

#include <algorithm>
#include <cmath>

namespace sdl3cpp::q3 {

Wish ComputeWish(float moveForward, float moveRight, float yaw,
                 float maxSpeed) {
    const float sinYaw = std::sin(yaw);
    const float cosYaw = std::cos(yaw);

    // forward = (-sin, 0, -cos); right = forward x up = (cos, 0, -sin)
    glm::vec3 direction{
        -sinYaw * moveForward + cosYaw * moveRight,
        0.0f,
        -cosYaw * moveForward - sinYaw * moveRight};

    const float length = std::sqrt(direction.x * direction.x +
                                   direction.z * direction.z);
    if (length < 0.001f) {
        return Wish{};
    }

    Wish wish;
    wish.direction = direction / length;
    wish.speed = maxSpeed * std::max(std::fabs(moveForward),
                                     std::fabs(moveRight));
    return wish;
}

}  // namespace sdl3cpp::q3
