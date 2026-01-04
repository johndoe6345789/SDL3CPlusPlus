#pragma once

#include "../interfaces/i_physics_service.hpp"
#include "../interfaces/i_logger.hpp"
#include "physics_bridge.hpp"
#include "../../di/lifecycle.hpp"
#include <memory>

namespace sdl3cpp::services::impl {

/**
 * @brief Bullet physics service implementation.
 *
 * Small wrapper service (~120 lines) around PhysicsBridge.
 * Provides rigid body physics simulation using Bullet Physics.
 */
class BulletPhysicsService : public IPhysicsService,
                             public di::IInitializable,
                             public di::IShutdownable {
public:
    explicit BulletPhysicsService(std::shared_ptr<ILogger> logger);
    ~BulletPhysicsService() override;

    // IInitializable interface
    void Initialize() override { Initialize(btVector3(0, -9.8f, 0)); }

    // IPhysicsService interface
    void Initialize(const btVector3& gravity = btVector3(0, -9.8f, 0)) override;
    void Shutdown() noexcept override;

    bool AddBoxRigidBody(const std::string& name,
                        const btVector3& halfExtents,
                        float mass,
                        const btTransform& transform) override;

    bool AddSphereRigidBody(const std::string& name,
                           float radius,
                           float mass,
                           const btTransform& transform) override;

    bool RemoveRigidBody(const std::string& name) override;

    void StepSimulation(float deltaTime, int maxSubSteps = 10) override;

    bool GetTransform(const std::string& name, btTransform& outTransform) const override;
    bool SetTransform(const std::string& name, const btTransform& transform) override;

    bool ApplyForce(const std::string& name, const btVector3& force) override;
    bool ApplyImpulse(const std::string& name, const btVector3& impulse) override;
    bool SetLinearVelocity(const std::string& name, const btVector3& velocity) override;

    size_t GetBodyCount() const override;
    void Clear() override;

private:
    std::shared_ptr<ILogger> logger_;
    std::unique_ptr<PhysicsBridge> physicsBridge_;
    bool initialized_ = false;
};

}  // namespace sdl3cpp::services::impl
