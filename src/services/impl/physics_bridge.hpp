#pragma once

#include <memory>
#include <string>
#include <unordered_map>

namespace sdl3cpp::services {
class ILogger;
}

class btVector3;
class btTransform;
class btCollisionShape;
class btMotionState;
class btRigidBody;
class btDefaultCollisionConfiguration;
class btCollisionDispatcher;
class btBroadphaseInterface;
class btSequentialImpulseConstraintSolver;
class btDiscreteDynamicsWorld;

namespace sdl3cpp::services::impl {

class PhysicsBridge {
public:
    explicit PhysicsBridge(std::shared_ptr<services::ILogger> logger);
    ~PhysicsBridge();

    PhysicsBridge(const PhysicsBridge&) = delete;
    PhysicsBridge& operator=(const PhysicsBridge&) = delete;

    bool addBoxRigidBody(const std::string& name,
                         const btVector3& halfExtents,
                         float mass,
                         const btTransform& transform,
                         std::string& error);
    int stepSimulation(float deltaTime);
    bool getRigidBodyTransform(const std::string& name,
                               btTransform& outTransform,
                               std::string& error) const;

private:
    struct BodyRecord {
        std::unique_ptr<btCollisionShape> shape;
        std::unique_ptr<btMotionState> motionState;
        std::unique_ptr<btRigidBody> body;
    };

    std::unique_ptr<btDefaultCollisionConfiguration> collisionConfig_;
    std::unique_ptr<btCollisionDispatcher> dispatcher_;
    std::unique_ptr<btBroadphaseInterface> broadphase_;
    std::unique_ptr<btSequentialImpulseConstraintSolver> solver_;
    std::unique_ptr<btDiscreteDynamicsWorld> world_;
    std::unordered_map<std::string, BodyRecord> bodies_;
    std::shared_ptr<services::ILogger> logger_;
};

} // namespace sdl3cpp::services::impl
