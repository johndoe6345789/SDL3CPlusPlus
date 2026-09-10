#include "services/interfaces/workflow/rendering/map_mesh_physics.hpp"

namespace sdl3cpp::services::impl {

btRigidBody* CreateMapMeshPhysicsBody(btDiscreteDynamicsWorld* world,
                                      const glm::vec3& bbMin,
                                      const glm::vec3& bbMax) {
    if (!world) return nullptr;

    const float cx = (bbMin.x + bbMax.x) * 0.5f;
    const float cy = (bbMin.y + bbMax.y) * 0.5f;
    const float cz = (bbMin.z + bbMax.z) * 0.5f;
    const float hx = (bbMax.x - bbMin.x) * 0.5f;
    const float hy = (bbMax.y - bbMin.y) * 0.5f;
    const float hz = (bbMax.z - bbMin.z) * 0.5f;
    if (hx <= 0.01f || hy <= 0.01f || hz <= 0.01f) return nullptr;

    auto* shape = new btBoxShape(btVector3(hx, hy, hz));
    btTransform startTransform;
    startTransform.setIdentity();
    startTransform.setOrigin(btVector3(cx, cy, cz));

    auto* motionState = new btDefaultMotionState(startTransform);
    btRigidBody::btRigidBodyConstructionInfo rbInfo(0.0f, motionState, shape);
    auto* body = new btRigidBody(rbInfo);
    world->addRigidBody(body);
    return body;
}

}  // namespace sdl3cpp::services::impl
