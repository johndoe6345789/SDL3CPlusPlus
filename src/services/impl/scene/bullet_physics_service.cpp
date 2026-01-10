#include "bullet_physics_service.hpp"
#include "services/interfaces/i_logger.hpp"
#include <stdexcept>

namespace sdl3cpp::services::impl {

BulletPhysicsService::BulletPhysicsService(std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)) {
    if (logger_) {
        logger_->Trace("BulletPhysicsService", "BulletPhysicsService");
    }
}

BulletPhysicsService::~BulletPhysicsService() {
    if (logger_) {
        logger_->Trace("BulletPhysicsService", "~BulletPhysicsService");
    }
    if (initialized_) {
        Shutdown();
    }
}

void BulletPhysicsService::Initialize(const btVector3& gravity) {
    logger_->Trace("BulletPhysicsService", "Initialize",
                   "gravity.x=" + std::to_string(gravity.getX()) +
                   ", gravity.y=" + std::to_string(gravity.getY()) +
                   ", gravity.z=" + std::to_string(gravity.getZ()));

    gravity_ = gravity;
    bool wasInitialized = initialized_;

    if (!initialized_) {
        physicsBridge_ = std::make_unique<PhysicsBridgeService>(logger_);
        initialized_ = true;
    }

    if (physicsBridge_) {
        std::string error;
        if (!physicsBridge_->SetGravity(gravity_, error)) {
            logger_->Warn("SetGravity failed: " + error);
        }
    }

    if (!wasInitialized && initialized_) {
        logger_->Info("Physics service initialized");
    }
}

void BulletPhysicsService::Shutdown() noexcept {
    logger_->Trace("BulletPhysicsService", "Shutdown");

    if (!initialized_) {
        return;
    }

    physicsBridge_.reset();
    initialized_ = false;

    logger_->Info("Physics service shutdown");
}

bool BulletPhysicsService::AddBoxRigidBody(const std::string& name,
                                          const btVector3& halfExtents,
                                          float mass,
                                          const btTransform& transform) {
    logger_->Trace("BulletPhysicsService", "AddBoxRigidBody",
                   "name=" + name +
                   ", halfExtents.x=" + std::to_string(halfExtents.getX()) +
                   ", halfExtents.y=" + std::to_string(halfExtents.getY()) +
                   ", halfExtents.z=" + std::to_string(halfExtents.getZ()) +
                   ", mass=" + std::to_string(mass) +
                   ", origin.x=" + std::to_string(transform.getOrigin().getX()) +
                   ", origin.y=" + std::to_string(transform.getOrigin().getY()) +
                   ", origin.z=" + std::to_string(transform.getOrigin().getZ()));

    if (!physicsBridge_) {
        throw std::runtime_error("Physics service not initialized");
    }

    std::string error;
    if (!physicsBridge_->AddBoxRigidBody(name, halfExtents, mass, transform, error)) {
        logger_->Error("AddBoxRigidBody failed: " + error);
        return false;
    }
    return true;
}

bool BulletPhysicsService::AddSphereRigidBody(const std::string& name,
                                             float radius,
                                             float mass,
                                             const btTransform& transform) {
    logger_->Trace("BulletPhysicsService", "AddSphereRigidBody",
                   "name=" + name +
                   ", radius=" + std::to_string(radius) +
                   ", mass=" + std::to_string(mass) +
                   ", origin.x=" + std::to_string(transform.getOrigin().getX()) +
                   ", origin.y=" + std::to_string(transform.getOrigin().getY()) +
                   ", origin.z=" + std::to_string(transform.getOrigin().getZ()));

    if (!physicsBridge_) {
        throw std::runtime_error("Physics service not initialized");
    }

    std::string error;
    if (!physicsBridge_->AddSphereRigidBody(name, radius, mass, transform, error)) {
        logger_->Error("AddSphereRigidBody failed: " + error);
        return false;
    }
    return true;
}

bool BulletPhysicsService::RemoveRigidBody(const std::string& name) {
    logger_->Trace("BulletPhysicsService", "RemoveRigidBody", "name=" + name);

    if (!physicsBridge_) {
        throw std::runtime_error("Physics service not initialized");
    }

    std::string error;
    if (!physicsBridge_->RemoveRigidBody(name, error)) {
        logger_->Error("RemoveRigidBody failed: " + error);
        return false;
    }
    return true;
}

void BulletPhysicsService::StepSimulation(float deltaTime, int maxSubSteps) {
    logger_->Trace("BulletPhysicsService", "StepSimulation",
                   "deltaTime=" + std::to_string(deltaTime) +
                   ", maxSubSteps=" + std::to_string(maxSubSteps));

    if (!physicsBridge_) {
        throw std::runtime_error("Physics service not initialized");
    }

    physicsBridge_->StepSimulation(deltaTime, maxSubSteps);
}

bool BulletPhysicsService::GetTransform(const std::string& name, btTransform& outTransform) const {
    logger_->Trace("BulletPhysicsService", "GetTransform", "name=" + name);
    if (!physicsBridge_) {
        return false;
    }

    std::string error;
    if (!physicsBridge_->GetRigidBodyTransform(name, outTransform, error)) {
        if (logger_) {
            logger_->Warn("GetTransform failed: " + error);
        }
        return false;
    }
    return true;
}

bool BulletPhysicsService::SetTransform(const std::string& name, const btTransform& transform) {
    logger_->Trace("BulletPhysicsService", "SetTransform",
                   "name=" + name +
                   ", origin.x=" + std::to_string(transform.getOrigin().getX()) +
                   ", origin.y=" + std::to_string(transform.getOrigin().getY()) +
                   ", origin.z=" + std::to_string(transform.getOrigin().getZ()));

    if (!physicsBridge_) {
        throw std::runtime_error("Physics service not initialized");
    }

    std::string error;
    if (!physicsBridge_->SetRigidBodyTransform(name, transform, error)) {
        logger_->Error("SetTransform failed: " + error);
        return false;
    }
    return true;
}

bool BulletPhysicsService::ApplyForce(const std::string& name, const btVector3& force) {
    logger_->Trace("BulletPhysicsService", "ApplyForce",
                   "name=" + name +
                   ", force.x=" + std::to_string(force.getX()) +
                   ", force.y=" + std::to_string(force.getY()) +
                   ", force.z=" + std::to_string(force.getZ()));

    if (!physicsBridge_) {
        throw std::runtime_error("Physics service not initialized");
    }

    std::string error;
    if (!physicsBridge_->ApplyForce(name, force, error)) {
        logger_->Error("ApplyForce failed: " + error);
        return false;
    }
    return true;
}

bool BulletPhysicsService::ApplyImpulse(const std::string& name, const btVector3& impulse) {
    logger_->Trace("BulletPhysicsService", "ApplyImpulse",
                   "name=" + name +
                   ", impulse.x=" + std::to_string(impulse.getX()) +
                   ", impulse.y=" + std::to_string(impulse.getY()) +
                   ", impulse.z=" + std::to_string(impulse.getZ()));

    if (!physicsBridge_) {
        throw std::runtime_error("Physics service not initialized");
    }

    std::string error;
    if (!physicsBridge_->ApplyImpulse(name, impulse, error)) {
        logger_->Error("ApplyImpulse failed: " + error);
        return false;
    }
    return true;
}

bool BulletPhysicsService::SetLinearVelocity(const std::string& name, const btVector3& velocity) {
    logger_->Trace("BulletPhysicsService", "SetLinearVelocity",
                   "name=" + name +
                   ", velocity.x=" + std::to_string(velocity.getX()) +
                   ", velocity.y=" + std::to_string(velocity.getY()) +
                   ", velocity.z=" + std::to_string(velocity.getZ()));

    if (!physicsBridge_) {
        throw std::runtime_error("Physics service not initialized");
    }

    std::string error;
    if (!physicsBridge_->SetLinearVelocity(name, velocity, error)) {
        logger_->Error("SetLinearVelocity failed: " + error);
        return false;
    }
    return true;
}

size_t BulletPhysicsService::GetBodyCount() const {
    logger_->Trace("BulletPhysicsService", "GetBodyCount");
    if (!physicsBridge_) {
        return 0;
    }
    return physicsBridge_->GetBodyCount();
}

void BulletPhysicsService::Clear() {
    logger_->Trace("BulletPhysicsService", "Clear");

    if (!physicsBridge_) {
        return;
    }

    physicsBridge_->Clear();
}

}  // namespace sdl3cpp::services::impl
