#include "physics_bridge_service.hpp"

#include <btBulletDynamicsCommon.h>

#include <utility>

namespace sdl3cpp::services::impl {

PhysicsBridgeService::PhysicsBridgeService(std::shared_ptr<ILogger> logger)
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
        logger_->Trace("PhysicsBridgeService", "PhysicsBridgeService");
    }
    world_->setGravity(btVector3(0.0f, -9.81f, 0.0f));
}

PhysicsBridgeService::~PhysicsBridgeService() {
    if (logger_) {
        logger_->Trace("PhysicsBridgeService", "~PhysicsBridgeService");
    }
    if (world_) {
        for (auto& [name, entry] : bodies_) {
            if (entry.body) {
                world_->removeRigidBody(entry.body.get());
            }
        }
    }
}

bool PhysicsBridgeService::AddBoxRigidBody(const std::string& name,
                                           const btVector3& halfExtents,
                                           float mass,
                                           const btTransform& transform,
                                           std::string& error) {
    if (logger_) {
        logger_->Trace("PhysicsBridgeService", "AddBoxRigidBody",
                       "name=" + name +
                       ", halfExtents.x=" + std::to_string(halfExtents.getX()) +
                       ", halfExtents.y=" + std::to_string(halfExtents.getY()) +
                       ", halfExtents.z=" + std::to_string(halfExtents.getZ()) +
                       ", mass=" + std::to_string(mass) +
                       ", origin.x=" + std::to_string(transform.getOrigin().getX()) +
                       ", origin.y=" + std::to_string(transform.getOrigin().getY()) +
                       ", origin.z=" + std::to_string(transform.getOrigin().getZ()));
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

int PhysicsBridgeService::StepSimulation(float deltaTime) {
    if (logger_) {
        logger_->Trace("PhysicsBridgeService", "StepSimulation", "deltaTime=" + std::to_string(deltaTime));
    }
    if (!world_) {
        return 0;
    }
    return static_cast<int>(world_->stepSimulation(deltaTime, 10, 1.0f / 60.0f));
}

bool PhysicsBridgeService::GetRigidBodyTransform(const std::string& name,
                                                 btTransform& outTransform,
                                                 std::string& error) const {
    if (logger_) {
        logger_->Trace("PhysicsBridgeService", "GetRigidBodyTransform", "name=" + name);
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

}  // namespace sdl3cpp::services::impl
