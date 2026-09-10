#pragma once

#include <btBulletDynamicsCommon.h>

namespace sdl3cpp::services::impl {

/// Removes `body` from `world` and deletes it, its motion state, and its
/// collision shape (recursing into a compound shape's child shapes).
void RemoveBspCollisionBody(btDiscreteDynamicsWorld* world, btRigidBody*& body);

/// Wraps `shape` in a static btRigidBody with the given friction, adds it
/// to `world` with the default collision filters, and returns it.
btRigidBody* AddStaticCollisionBody(btDiscreteDynamicsWorld* world,
                                    btCollisionShape* shape, float friction);

/// Same as AddStaticCollisionBody, but adds the body with an explicit
/// collision `group`/`mask` pair (used for the player-clip body, which
/// only the character controller's queries should see).
btRigidBody* AddFilteredStaticCollisionBody(btDiscreteDynamicsWorld* world,
                                            btCollisionShape* shape, int group,
                                            int mask);

}  // namespace sdl3cpp::services::impl
