#pragma once

#include <string>
#include <LinearMath/btVector3.h>
#include <LinearMath/btTransform.h>

namespace sdl3cpp::services {

/**
 * @brief Physics simulation service interface.
 *
 * Provides rigid body physics simulation using Bullet Physics.
 * Wraps the physics bridge service with a clean simulation interface.
 */
class IPhysicsService {
public:
    virtual ~IPhysicsService() = default;

    /**
     * @brief Initialize the physics world.
     *
     * @param gravity Gravity vector (default: {0, -9.8, 0})
     */
    virtual void Initialize(const btVector3& gravity = btVector3(0, -9.8f, 0)) = 0;

    /**
     * @brief Shutdown and release physics resources.
     */
    virtual void Shutdown() = 0;

    /**
     * @brief Add a box-shaped rigid body to the simulation.
     *
     * @param name Unique identifier for this body
     * @param halfExtents Half-extents of the box (half width/height/depth)
     * @param mass Mass in kg (0 for static objects)
     * @param transform Initial position and rotation
     * @return true if added successfully, false if name already exists
     */
    virtual bool AddBoxRigidBody(const std::string& name,
                                 const btVector3& halfExtents,
                                 float mass,
                                 const btTransform& transform) = 0;

    /**
     * @brief Add a sphere-shaped rigid body to the simulation.
     *
     * @param name Unique identifier for this body
     * @param radius Sphere radius
     * @param mass Mass in kg (0 for static objects)
     * @param transform Initial position and rotation
     * @return true if added successfully, false if name already exists
     */
    virtual bool AddSphereRigidBody(const std::string& name,
                                    float radius,
                                    float mass,
                                    const btTransform& transform) = 0;

    /**
     * @brief Remove a rigid body from the simulation.
     *
     * @param name Identifier of the body to remove
     * @return true if removed, false if not found
     */
    virtual bool RemoveRigidBody(const std::string& name) = 0;

    /**
     * @brief Step the physics simulation forward in time.
     *
     * @param deltaTime Time step in seconds (typically 1/60)
     * @param maxSubSteps Maximum number of sub-steps for stability
     */
    virtual void StepSimulation(float deltaTime, int maxSubSteps = 10) = 0;

    /**
     * @brief Get the current transform of a rigid body.
     *
     * @param name Identifier of the body
     * @param outTransform Output parameter for the transform
     * @return true if body exists, false otherwise
     */
    virtual bool GetTransform(const std::string& name, btTransform& outTransform) const = 0;

    /**
     * @brief Set the transform of a rigid body.
     *
     * @param name Identifier of the body
     * @param transform New position and rotation
     * @return true if body exists, false otherwise
     */
    virtual bool SetTransform(const std::string& name, const btTransform& transform) = 0;

    /**
     * @brief Apply a force to a rigid body.
     *
     * @param name Identifier of the body
     * @param force Force vector in Newtons
     * @return true if body exists, false otherwise
     */
    virtual bool ApplyForce(const std::string& name, const btVector3& force) = 0;

    /**
     * @brief Apply an impulse to a rigid body.
     *
     * @param name Identifier of the body
     * @param impulse Impulse vector
     * @return true if body exists, false otherwise
     */
    virtual bool ApplyImpulse(const std::string& name, const btVector3& impulse) = 0;

    /**
     * @brief Set the linear velocity of a rigid body.
     *
     * @param name Identifier of the body
     * @param velocity Velocity vector in m/s
     * @return true if body exists, false otherwise
     */
    virtual bool SetLinearVelocity(const std::string& name, const btVector3& velocity) = 0;

    /**
     * @brief Get the number of rigid bodies in the simulation.
     *
     * @return Body count
     */
    virtual size_t GetBodyCount() const = 0;

    /**
     * @brief Clear all rigid bodies from the simulation.
     */
    virtual void Clear() = 0;
};

}  // namespace sdl3cpp::services
