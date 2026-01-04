#include "physics_bridge.hpp"
#include "../interfaces/i_logger.hpp"

#include <btBulletDynamicsCommon.h>

namespace sdl3cpp::services::impl {

PhysicsBridge::PhysicsBridge(std::shared_ptr<services::ILogger> logger)
    : collisionConfig_(std::make_unique<btDefaultCollisionConfiguration>()),
      dispatcher_(std::make_unique<btCollisionDispatcher>(collisionConfig_.get())),
      broadphase_(std::make_unique<btDbvtBroadphase>()),
      solver_(std::make_unique<btSequentialImpulseConstraintSolver>()),
      world_(std::make_unique<btDiscreteDynamicsWorld>(
          dispatcher_.get(),
          broadphase_.get(),
          solver_.get(),
          collisionConfig_.get())),
      logger_(std::move(logger)) {
    if (logger_) {
        logger_->Trace("PhysicsBridge", "PhysicsBridge");
    }
    world_->setGravity(btVector3(0.0f, -9.81f, 0.0f));
}

PhysicsBridge::~PhysicsBridge() {
    if (world_) {
        for (auto& [name, entry] : bodies_) {
            if (entry.body) {
                world_->removeRigidBody(entry.body.get());
            }
        }
    }
}

bool PhysicsBridge::addBoxRigidBody(const std::string& name,
                                    const btVector3& halfExtents,
                                    float mass,
                                    const btTransform& transform,
                                    std::string& error) {
    if (logger_) {
        logger_->Trace("PhysicsBridge", "addBoxRigidBody", "name=" + name);
    }
    if (name.empty()) {
        error = "Rigid body name must not be empty";
        return false;
    }
    if (!world_) {
        error = "Physics world is not initialized";
        return false;
    }
    if (bodies_.count(name)) {
        error = "Rigid body already exists: " + name;
        return false;
    }
    auto shape = std::make_unique<btBoxShape>(halfExtents);
    btVector3 inertia(0.0f, 0.0f, 0.0f);
    if (mass > 0.0f) {
        shape->calculateLocalInertia(mass, inertia);
    }
    auto motionState = std::make_unique<btDefaultMotionState>(transform);
    btRigidBody::btRigidBodyConstructionInfo constructionInfo(
        mass,
        motionState.get(),
        shape.get(),
        inertia);
    auto body = std::make_unique<btRigidBody>(constructionInfo);
    world_->addRigidBody(body.get());
    bodies_.emplace(name, BodyRecord{
        std::move(shape),
        std::move(motionState),
        std::move(body),
    });
    return true;
}

int PhysicsBridge::stepSimulation(float deltaTime) {
    if (logger_) {
        logger_->Trace("PhysicsBridge", "stepSimulation", "deltaTime=" + std::to_string(deltaTime));
    }
    if (!world_) {
        return 0;
    }
    return static_cast<int>(world_->stepSimulation(deltaTime, 10, 1.0f / 60.0f));
}

bool PhysicsBridge::getRigidBodyTransform(const std::string& name,
                                          btTransform& outTransform,
                                          std::string& error) const {
    if (logger_) {
        logger_->Trace("PhysicsBridge", "getRigidBodyTransform", "name=" + name);
    }
    auto it = bodies_.find(name);
    if (it == bodies_.end()) {
        error = "Rigid body not found: " + name;
        return false;
    }
    if (!it->second.motionState) {
        error = "Rigid body motion state is missing";
        return false;
    }
    it->second.motionState->getWorldTransform(outTransform);
    return true;
}

} // namespace sdl3cpp::services::impl
