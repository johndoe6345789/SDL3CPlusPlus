#include "bullet_physics_service.hpp"
#include "../interfaces/i_logger.hpp"
#include <stdexcept>

namespace sdl3cpp::services::impl {

BulletPhysicsService::BulletPhysicsService(std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)) {
}

BulletPhysicsService::~BulletPhysicsService() {
    if (initialized_) {
        Shutdown();
    }
}

void BulletPhysicsService::Initialize(const btVector3& gravity) {
    logger_->TraceFunction(__func__);

    if (initialized_) {
        return;
    }

    physicsBridge_ = std::make_unique<PhysicsBridgeService>(logger_);
    initialized_ = true;

    logger_->Info("Physics service initialized");
}

void BulletPhysicsService::Shutdown() noexcept {
    logger_->TraceFunction(__func__);

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
    logger_->TraceFunction(__func__);

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
    logger_->TraceFunction(__func__);

    // PhysicsBridgeService doesn't support sphere rigid bodies in current implementation
    logger_->Warn("AddSphereRigidBody not supported by PhysicsBridgeService");
    return false;
}

bool BulletPhysicsService::RemoveRigidBody(const std::string& name) {
    logger_->TraceFunction(__func__);

    // PhysicsBridgeService doesn't support removing bodies in current implementation
    logger_->Warn("RemoveRigidBody not supported by PhysicsBridgeService");
    return false;
}

void BulletPhysicsService::StepSimulation(float deltaTime, int maxSubSteps) {
    logger_->TraceFunction(__func__);

    if (!physicsBridge_) {
        throw std::runtime_error("Physics service not initialized");
    }

    physicsBridge_->StepSimulation(deltaTime);
}

bool BulletPhysicsService::GetTransform(const std::string& name, btTransform& outTransform) const {
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
    logger_->TraceFunction(__func__);

    // PhysicsBridgeService doesn't support setting transforms in current implementation
    logger_->Warn("SetTransform not supported by PhysicsBridgeService");
    return false;
}

bool BulletPhysicsService::ApplyForce(const std::string& name, const btVector3& force) {
    logger_->TraceFunction(__func__);

    // PhysicsBridgeService doesn't support applying forces in current implementation
    logger_->Warn("ApplyForce not supported by PhysicsBridgeService");
    return false;
}

bool BulletPhysicsService::ApplyImpulse(const std::string& name, const btVector3& impulse) {
    logger_->TraceFunction(__func__);

    // PhysicsBridgeService doesn't support applying impulses in current implementation
    logger_->Warn("ApplyImpulse not supported by PhysicsBridgeService");
    return false;
}

bool BulletPhysicsService::SetLinearVelocity(const std::string& name, const btVector3& velocity) {
    logger_->TraceFunction(__func__);

    // PhysicsBridgeService doesn't support setting velocity in current implementation
    logger_->Warn("SetLinearVelocity not supported by PhysicsBridgeService");
    return false;
}

size_t BulletPhysicsService::GetBodyCount() const {
    // PhysicsBridgeService doesn't expose GetBodyCount in current implementation
    // Returning 0 as stub - could track bodies in wrapper if needed
    return 0;
}

void BulletPhysicsService::Clear() {
    logger_->TraceFunction(__func__);

    if (!physicsBridge_) {
        return;
    }

    // PhysicsBridgeService doesn't expose Clear in current implementation
    // Shutdown and reinitialize to clear all bodies
    physicsBridge_.reset();
    physicsBridge_ = std::make_unique<PhysicsBridgeService>(logger_);
}

}  // namespace sdl3cpp::services::impl
