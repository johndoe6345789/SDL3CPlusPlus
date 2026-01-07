#pragma once

#include <cstddef>
#include <string>

class btVector3;
class btTransform;

namespace sdl3cpp::services {

/**
 * @brief Script-facing physics bridge service interface.
 */
class IPhysicsBridgeService {
public:
    virtual ~IPhysicsBridgeService() = default;

    virtual bool SetGravity(const btVector3& gravity,
                            std::string& error) = 0;

    virtual bool AddBoxRigidBody(const std::string& name,
                                 const btVector3& halfExtents,
                                 float mass,
                                 const btTransform& transform,
                                 std::string& error) = 0;
    virtual bool AddSphereRigidBody(const std::string& name,
                                    float radius,
                                    float mass,
                                    const btTransform& transform,
                                    std::string& error) = 0;
    virtual bool RemoveRigidBody(const std::string& name,
                                 std::string& error) = 0;
    virtual bool SetRigidBodyTransform(const std::string& name,
                                       const btTransform& transform,
                                       std::string& error) = 0;
    virtual bool ApplyForce(const std::string& name,
                            const btVector3& force,
                            std::string& error) = 0;
    virtual bool ApplyImpulse(const std::string& name,
                              const btVector3& impulse,
                              std::string& error) = 0;
    virtual bool SetLinearVelocity(const std::string& name,
                                   const btVector3& velocity,
                                   std::string& error) = 0;
    virtual int StepSimulation(float deltaTime, int maxSubSteps = 10) = 0;
    virtual bool GetRigidBodyTransform(const std::string& name,
                                       btTransform& outTransform,
                                       std::string& error) const = 0;
    virtual size_t GetBodyCount() const = 0;
    virtual void Clear() = 0;
};

}  // namespace sdl3cpp::services
