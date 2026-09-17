#include "services/interfaces/workflow/fs2024/terrain/fs2024_terrain_collision.hpp"

#include <BulletCollision/CollisionShapes/btHeightfieldTerrainShape.h>

namespace sdl3cpp::services::impl {

Fs2024TerrainCollision AddFs2024TerrainCollision(
    btDiscreteDynamicsWorld* world, const Fs2024Heightfield& field) {
    Fs2024TerrainCollision collision;
    constexpr int kUpAxisY = 1;
    collision.shape = new btHeightfieldTerrainShape(
        field.columns, field.rows, field.heights.data(), field.minHeight,
        field.maxHeight, kUpAxisY, false);
    // Grid steps are one unit before scaling; heights are already metres.
    collision.shape->setLocalScaling(
        btVector3(field.spacing, 1.f, field.spacing));

    // Bullet centres the shape on its bounding box, so the body sits at
    // the middle of the grid and halfway up its height range.
    const float halfX = 0.5f * field.spacing *
                        static_cast<float>(field.columns - 1);
    const float halfZ = 0.5f * field.spacing *
                        static_cast<float>(field.rows - 1);
    btTransform transform;
    transform.setIdentity();
    transform.setOrigin(btVector3(
        field.origin.x + halfX, 0.5f * (field.minHeight + field.maxHeight),
        field.origin.y + halfZ));

    collision.motion = new btDefaultMotionState(transform);
    btRigidBody::btRigidBodyConstructionInfo info(0.f, collision.motion,
                                                  collision.shape);
    info.m_friction = 1.f;
    collision.body = new btRigidBody(info);
    collision.body->setCollisionFlags(collision.body->getCollisionFlags() |
                                      btCollisionObject::CF_STATIC_OBJECT);
    if (world) world->addRigidBody(collision.body);
    return collision;
}

void RemoveFs2024TerrainCollision(btDiscreteDynamicsWorld* world,
                                  Fs2024TerrainCollision& collision) {
    if (world && collision.body) world->removeRigidBody(collision.body);
    delete collision.body;
    delete collision.motion;
    delete collision.shape;
    collision = Fs2024TerrainCollision{};
}

}  // namespace sdl3cpp::services::impl
