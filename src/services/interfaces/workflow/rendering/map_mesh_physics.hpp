#pragma once

#include <btBulletDynamicsCommon.h>
#include <glm/glm.hpp>

namespace sdl3cpp::services::impl {

/// Builds a static (mass 0) box body sized to [bbMin, bbMax] and adds it to
/// `world`; returns null when the box would be degenerate (any axis
/// smaller than 0.01) or `world` is null.
btRigidBody* CreateMapMeshPhysicsBody(btDiscreteDynamicsWorld* world,
                                      const glm::vec3& bbMin,
                                      const glm::vec3& bbMax);

}  // namespace sdl3cpp::services::impl
