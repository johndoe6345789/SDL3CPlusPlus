#include "services/interfaces/workflow/gta5/world/gta5_collision_body.hpp"

#include <cmath>

namespace sdl3cpp::services::impl {
namespace {

bool IsUnitScale(const glm::vec3& scale) {
    constexpr float kEpsilon = 0.001f;
    return std::fabs(scale.x - 1.f) < kEpsilon &&
           std::fabs(scale.y - 1.f) < kEpsilon &&
           std::fabs(scale.z - 1.f) < kEpsilon;
}

}  // namespace

bool AddGta5InstanceBody(btDiscreteDynamicsWorld* world,
                         const Gta5Placement& placement,
                         Gta5Geometry& geometry, Gta5Instance& instance) {
    if (!world || !geometry.collisionShape) return false;

    btCollisionShape* shape = geometry.collisionShape;
    if (!IsUnitScale(placement.scale)) {
        instance.scaledShape = new btScaledBvhTriangleMeshShape(
            geometry.collisionShape,
            btVector3(placement.scale.x, placement.scale.y,
                      placement.scale.z));
        shape = instance.scaledShape;
    }

    btTransform transform;
    transform.setIdentity();
    transform.setOrigin(btVector3(placement.position.x, placement.position.y,
                                  placement.position.z));
    transform.setRotation(btQuaternion(placement.rotation.x,
                                       placement.rotation.y,
                                       placement.rotation.z,
                                       placement.rotation.w));

    auto* motionState = new btDefaultMotionState(transform);
    // Mass zero is what makes it static: the world moves around it.
    btRigidBody::btRigidBodyConstructionInfo info(0.f, motionState, shape);
    info.m_friction = 1.f;
    auto* body = new btRigidBody(info);
    body->setCollisionFlags(body->getCollisionFlags() |
                            btCollisionObject::CF_STATIC_OBJECT);
    world->addRigidBody(body);

    instance.body = body;
    return true;
}

void RemoveGta5InstanceBody(btDiscreteDynamicsWorld* world,
                            Gta5Instance& instance) {
    if (instance.body && world) {
        world->removeRigidBody(instance.body);
        delete instance.body->getMotionState();
        delete instance.body;
    }
    instance.body = nullptr;
    // The shared archetype shape is not ours to free; a scaled wrapper is.
    delete instance.scaledShape;
    instance.scaledShape = nullptr;
}

}  // namespace sdl3cpp::services::impl
