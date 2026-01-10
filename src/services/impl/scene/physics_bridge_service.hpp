#pragma once

#include "services/interfaces/i_physics_bridge_service.hpp"
#include "services/interfaces/i_logger.hpp"
#include <array>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

class btVector3;
class btTransform;
class btCollisionShape;
class btMotionState;
class btRigidBody;
class btTriangleMesh;
class btDefaultCollisionConfiguration;
class btCollisionDispatcher;
class btBroadphaseInterface;
class btSequentialImpulseConstraintSolver;
class btDiscreteDynamicsWorld;

namespace sdl3cpp::services::impl {

/**
 * @brief Script-facing physics bridge service implementation.
 */
class PhysicsBridgeService : public IPhysicsBridgeService {
public:
    explicit PhysicsBridgeService(std::shared_ptr<ILogger> logger);
    ~PhysicsBridgeService() override;

    bool SetGravity(const btVector3& gravity,
                    std::string& error) override;

    bool AddBoxRigidBody(const std::string& name,
                         const btVector3& halfExtents,
                         float mass,
                         const btTransform& transform,
                         std::string& error) override;
    bool AddSphereRigidBody(const std::string& name,
                            float radius,
                            float mass,
                            const btTransform& transform,
                            std::string& error) override;
    bool AddTriangleMeshRigidBody(const std::string& name,
                                  const std::vector<std::array<float, 3>>& vertices,
                                  const std::vector<uint32_t>& indices,
                                  const btTransform& transform,
                                  std::string& error) override;
    bool RemoveRigidBody(const std::string& name,
                         std::string& error) override;
    bool SetRigidBodyTransform(const std::string& name,
                               const btTransform& transform,
                               std::string& error) override;
    bool ApplyForce(const std::string& name,
                    const btVector3& force,
                    std::string& error) override;
    bool ApplyImpulse(const std::string& name,
                      const btVector3& impulse,
                      std::string& error) override;
    bool SetLinearVelocity(const std::string& name,
                           const btVector3& velocity,
                           std::string& error) override;
    bool GetLinearVelocity(const std::string& name,
                           btVector3& outVelocity,
                           std::string& error) const override;
    int StepSimulation(float deltaTime, int maxSubSteps = 10) override;
    bool GetRigidBodyTransform(const std::string& name,
                               btTransform& outTransform,
                               std::string& error) const override;
    size_t GetBodyCount() const override;
    void Clear() override;

private:
    struct BodyRecord {
        std::unique_ptr<btCollisionShape> shape;
        std::unique_ptr<btMotionState> motionState;
        std::unique_ptr<btRigidBody> body;
        std::unique_ptr<btTriangleMesh> triangleMesh;
    };

    BodyRecord* FindBodyRecord(const std::string& name, std::string& error);
    const BodyRecord* FindBodyRecord(const std::string& name, std::string& error) const;

    std::unique_ptr<btDefaultCollisionConfiguration> collisionConfig_;
    std::unique_ptr<btCollisionDispatcher> dispatcher_;
    std::unique_ptr<btBroadphaseInterface> broadphase_;
    std::unique_ptr<btSequentialImpulseConstraintSolver> solver_;
    std::unique_ptr<btDiscreteDynamicsWorld> world_;
    std::unordered_map<std::string, BodyRecord> bodies_;
    std::shared_ptr<ILogger> logger_;
};

}  // namespace sdl3cpp::services::impl
