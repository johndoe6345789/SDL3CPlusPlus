#include "services/interfaces/workflow/switchback/terrain/switchback_terrain_state.hpp"

#include <BulletCollision/CollisionShapes/btHeightfieldTerrainShape.h>
#include <btBulletDynamicsCommon.h>

namespace sdl3cpp::services::impl {

void BuildSwitchbackCollision(btDiscreteDynamicsWorld& world,
                              const SwitchbackHeightmap& map, float stepM,
                              float heightMaxM, SwitchbackTerrainState& state) {
    // Bullet keeps a pointer into the height data, so it must outlive the body.
    state.collisionHeights = map.metres;
    state.shape = new btHeightfieldTerrainShape(
        map.size, map.size, state.collisionHeights.data(), 1.f, 0.f,
        heightMaxM, 1, PHY_FLOAT, false);
    state.shape->setLocalScaling(btVector3(stepM, 1.f, stepM));
    // Bullet subtracts the mid-height of the range from every sample, so the
    // body must sit at that mid-height to put the samples back at their metres.
    btTransform transform;
    transform.setIdentity();
    transform.setOrigin(btVector3(0.f, 0.5f * heightMaxM, 0.f));
    btRigidBody::btRigidBodyConstructionInfo info(0.f, nullptr, state.shape);
    state.body = new btRigidBody(info);
    state.body->setWorldTransform(transform);
    state.body->setFriction(0.9f);
    world.addRigidBody(state.body);
}

}  // namespace sdl3cpp::services::impl
