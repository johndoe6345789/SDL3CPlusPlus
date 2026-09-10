#include "services/interfaces/workflow/rendering/bsp_collision_body.hpp"

namespace sdl3cpp::services::impl {

void RemoveBspCollisionBody(btDiscreteDynamicsWorld* world,
                            btRigidBody*& body) {
    if (!body) return;

    world->removeRigidBody(body);
    auto* shape = body->getCollisionShape();
    auto* ms    = body->getMotionState();
    delete body;
    delete ms;
    if (shape) {
        if (auto* compound = dynamic_cast<btCompoundShape*>(shape)) {
            for (int i = compound->getNumChildShapes() - 1; i >= 0; --i) {
                delete compound->getChildShape(i);
            }
        }
        delete shape;
    }
    body = nullptr;
}

btRigidBody* AddStaticCollisionBody(btDiscreteDynamicsWorld* world,
                                    btCollisionShape* shape, float friction) {
    btTransform startTransform;
    startTransform.setIdentity();
    auto* motionState = new btDefaultMotionState(startTransform);
    btRigidBody::btRigidBodyConstructionInfo rbInfo(0.0f, motionState, shape);
    rbInfo.m_friction = friction;
    auto* body        = new btRigidBody(rbInfo);
    body->setCollisionFlags(body->getCollisionFlags() |
                            btCollisionObject::CF_STATIC_OBJECT);
    world->addRigidBody(body);
    return body;
}

btRigidBody* AddFilteredStaticCollisionBody(btDiscreteDynamicsWorld* world,
                                            btCollisionShape* shape, int group,
                                            int mask) {
    btTransform startTransform;
    startTransform.setIdentity();
    auto* motionState = new btDefaultMotionState(startTransform);
    btRigidBody::btRigidBodyConstructionInfo rbInfo(0.0f, motionState, shape);
    auto* body = new btRigidBody(rbInfo);
    body->setCollisionFlags(body->getCollisionFlags() |
                            btCollisionObject::CF_STATIC_OBJECT);
    world->addRigidBody(body, group, mask);
    return body;
}

}  // namespace sdl3cpp::services::impl
