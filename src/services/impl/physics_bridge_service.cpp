#include "physics_bridge_service.hpp"
#include <utility>

namespace sdl3cpp::services::impl {

PhysicsBridgeService::PhysicsBridgeService(std::shared_ptr<ILogger> logger)
    : bridge_(std::make_unique<PhysicsBridge>(logger)),
      logger_(std::move(logger)) {
    if (logger_) {
        logger_->Trace("PhysicsBridgeService", "PhysicsBridgeService");
    }
}

bool PhysicsBridgeService::AddBoxRigidBody(const std::string& name,
                                           const btVector3& halfExtents,
                                           float mass,
                                           const btTransform& transform,
                                           std::string& error) {
    if (logger_) {
        logger_->Trace("PhysicsBridgeService", "AddBoxRigidBody", "name=" + name);
    }
    if (!bridge_) {
        error = "Physics bridge not initialized";
        return false;
    }
    return bridge_->addBoxRigidBody(name, halfExtents, mass, transform, error);
}

int PhysicsBridgeService::StepSimulation(float deltaTime) {
    if (logger_) {
        logger_->Trace("PhysicsBridgeService", "StepSimulation", "deltaTime=" + std::to_string(deltaTime));
    }
    if (!bridge_) {
        return 0;
    }
    return bridge_->stepSimulation(deltaTime);
}

bool PhysicsBridgeService::GetRigidBodyTransform(const std::string& name,
                                                 btTransform& outTransform,
                                                 std::string& error) const {
    if (logger_) {
        logger_->Trace("PhysicsBridgeService", "GetRigidBodyTransform", "name=" + name);
    }
    if (!bridge_) {
        error = "Physics bridge not initialized";
        return false;
    }
    return bridge_->getRigidBodyTransform(name, outTransform, error);
}

}  // namespace sdl3cpp::services::impl
