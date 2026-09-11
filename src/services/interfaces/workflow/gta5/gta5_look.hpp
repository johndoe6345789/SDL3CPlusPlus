#pragma once

#include <btBulletDynamicsCommon.h>
#include <glm/glm.hpp>

#include <cmath>

namespace sdl3cpp::services::impl {

/// Where camera_yaw and camera_pitch look, as camera.fps.update turns
/// them into its view: yaw 0 faces -z, and yaw grows to the left.
inline glm::vec3 Gta5LookFront(float yaw, float pitch) {
    return {std::cos(pitch) * -std::sin(yaw), std::sin(pitch),
            std::cos(pitch) * -std::cos(yaw)};
}

/// The view's right, level with the ground.
inline glm::vec3 Gta5LookRight(float yaw) {
    return {std::cos(yaw), 0.f, -std::sin(yaw)};
}

/// `eye`, or, when something solid is between it and `head`, a point
/// just in front of that -- the player's own capsule never counts.
glm::vec3 Gta5ClearEye(btDiscreteDynamicsWorld* world, const glm::vec3& head,
                       const glm::vec3& eye, const btCollisionObject* player);

/// Whether a ray from `from` to `to` hits anything but `skip`; the first
/// hit's point goes to `at` when it is given.
bool Gta5RayHit(btDiscreteDynamicsWorld* world, const glm::vec3& from,
                const glm::vec3& to, const btCollisionObject* skip,
                glm::vec3* at);

}  // namespace sdl3cpp::services::impl
