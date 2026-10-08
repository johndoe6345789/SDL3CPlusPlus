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
    btRigidBody::btRigidBodyConstructionInfo info(0.f, nullptr, state.shape);
    state.body = new btRigidBody(info);
    state.body->setFriction(0.9f);
    world.addRigidBody(state.body);
}

}  // namespace sdl3cpp::services::impl
