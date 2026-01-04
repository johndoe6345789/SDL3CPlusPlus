#pragma once

#include "../interfaces/i_physics_bridge_service.hpp"
#include "../interfaces/i_logger.hpp"
#include "../../script/physics_bridge.hpp"
#include <memory>

namespace sdl3cpp::services::impl {

/**
 * @brief Script-facing physics bridge service implementation.
 */
class PhysicsBridgeService : public IPhysicsBridgeService {
public:
    explicit PhysicsBridgeService(std::shared_ptr<ILogger> logger);

    bool AddBoxRigidBody(const std::string& name,
                         const btVector3& halfExtents,
                         float mass,
                         const btTransform& transform,
                         std::string& error) override;
    int StepSimulation(float deltaTime) override;
    bool GetRigidBodyTransform(const std::string& name,
                               btTransform& outTransform,
                               std::string& error) const override;

private:
    std::unique_ptr<script::PhysicsBridge> bridge_;
    std::shared_ptr<ILogger> logger_;
};

}  // namespace sdl3cpp::services::impl
