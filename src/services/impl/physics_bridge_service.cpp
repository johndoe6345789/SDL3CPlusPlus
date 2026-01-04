#include "physics_bridge_service.hpp"
#include <utility>

namespace sdl3cpp::services::impl {

PhysicsBridgeService::PhysicsBridgeService(std::shared_ptr<IScriptEngineService> engineService)
    : engineService_(std::move(engineService)) {
}

bool PhysicsBridgeService::AddBoxRigidBody(const std::string& name,
                                           const btVector3& halfExtents,
                                           float mass,
                                           const btTransform& transform,
                                           std::string& error) {
    return engineService_->GetEngine().GetPhysicsBridge().addBoxRigidBody(name, halfExtents, mass, transform, error);
}

int PhysicsBridgeService::StepSimulation(float deltaTime) {
    return engineService_->GetEngine().GetPhysicsBridge().stepSimulation(deltaTime);
}

bool PhysicsBridgeService::GetRigidBodyTransform(const std::string& name,
                                                 btTransform& outTransform,
                                                 std::string& error) const {
    return engineService_->GetEngine().GetPhysicsBridge().getRigidBodyTransform(name, outTransform, error);
}

}  // namespace sdl3cpp::services::impl
