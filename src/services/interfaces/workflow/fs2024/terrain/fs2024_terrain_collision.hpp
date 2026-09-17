#pragma once

#include "services/interfaces/workflow/fs2024/terrain/fs2024_heightfield.hpp"

#include <btBulletDynamicsCommon.h>

class btHeightfieldTerrainShape;

namespace sdl3cpp::services::impl {

/// The ground as one static Bullet heightfield. The shape reads the
/// heights in place, so the field must outlive it.
struct Fs2024TerrainCollision {
    btHeightfieldTerrainShape* shape = nullptr;
    btDefaultMotionState* motion = nullptr;
    btRigidBody* body = nullptr;
};

/// Build the shape over `field` and add it to `world` as a static body.
/// A heightfield rather than a triangle mesh: a million-vertex BVH costs
/// tens of megabytes and seconds to build, a heightfield costs neither,
/// and it splits each cell on the same diagonal the drawn mesh does.
Fs2024TerrainCollision AddFs2024TerrainCollision(
    btDiscreteDynamicsWorld* world, const Fs2024Heightfield& field);

/// Remove the body from `world` (when there is one) and free it all.
void RemoveFs2024TerrainCollision(btDiscreteDynamicsWorld* world,
                                  Fs2024TerrainCollision& collision);

}  // namespace sdl3cpp::services::impl
