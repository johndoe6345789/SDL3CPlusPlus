#pragma once

#include <glm/glm.hpp>

class btCollisionObject;
class btDiscreteDynamicsWorld;

namespace sdl3cpp::services::impl {

/// Where a shot struck: the point, the surface it met, and what was
/// there -- a car takes the hit and rocks with it.
struct Gta5Shot {
    glm::vec3 at{0.f};
    glm::vec3 normal{0.f};
    const btCollisionObject* object{nullptr};
};

/// Where the gun is, which is not where the eye is: in third person
/// the camera orbits metres behind the player, so a flash hung off it
/// goes off in the lens. Shoulder high, a little forward and to the
/// right of `origin`, whose capsule reaches `head` above it.
glm::vec3 Gta5MuzzlePoint(const glm::vec3& eye, const glm::vec3& ahead,
                          const glm::vec3& origin, float head,
                          bool thirdPerson);

/// The first thing between `from` and `to`, `skip` aside. False when
/// the round meets nothing.
bool Gta5ShootRay(btDiscreteDynamicsWorld* world, const glm::vec3& from,
                  const glm::vec3& to, const btCollisionObject* skip,
                  Gta5Shot& shot);

}  // namespace sdl3cpp::services::impl
