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

/// The first thing between `from` and `to`, `skip` aside. False when
/// the round meets nothing.
bool Gta5ShootRay(btDiscreteDynamicsWorld* world, const glm::vec3& from,
                  const glm::vec3& to, const btCollisionObject* skip,
                  Gta5Shot& shot);

}  // namespace sdl3cpp::services::impl
