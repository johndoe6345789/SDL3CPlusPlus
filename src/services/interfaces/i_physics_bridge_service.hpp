#pragma once

#include "../../script/physics_bridge.hpp"
#include <string>

namespace sdl3cpp::services {

/**
 * @brief Script-facing physics bridge service interface.
 */
class IPhysicsBridgeService {
public:
    virtual ~IPhysicsBridgeService() = default;

    virtual bool AddBoxRigidBody(const std::string& name,
                                 const btVector3& halfExtents,
                                 float mass,
                                 const btTransform& transform,
                                 std::string& error) = 0;
    virtual int StepSimulation(float deltaTime) = 0;
    virtual bool GetRigidBodyTransform(const std::string& name,
                                       btTransform& outTransform,
                                       std::string& error) const = 0;
};

}  // namespace sdl3cpp::services
