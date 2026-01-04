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

    physicsBridge_ = std::make_unique<script::PhysicsBridge>();
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
    return physicsBridge_->addBoxRigidBody(name, halfExtents, mass, transform, error);
}

bool BulletPhysicsService::AddSphereRigidBody(const std::string& name,
                                             float radius,
                                             float mass,
                                             const btTransform& transform) {
    logger_->TraceFunction(__func__);

    // PhysicsBridge doesn't support sphere rigid bodies in current implementation
    logger_->Warn("AddSphereRigidBody not supported by PhysicsBridge");
    return false;
}

bool BulletPhysicsService::RemoveRigidBody(const std::string& name) {
    logger_->TraceFunction(__func__);

    // PhysicsBridge doesn't support removing bodies in current implementation
    logger_->Warn("RemoveRigidBody not supported by PhysicsBridge");
    return false;
}

void BulletPhysicsService::StepSimulation(float deltaTime, int maxSubSteps) {
    logger_->TraceFunction(__func__);

    if (!physicsBridge_) {
        throw std::runtime_error("Physics service not initialized");
    }

    physicsBridge_->stepSimulation(deltaTime);
}

bool BulletPhysicsService::GetTransform(const std::string& name, btTransform& outTransform) const {
    if (!physicsBridge_) {
        return false;
    }

    std::string error;
    return physicsBridge_->getRigidBodyTransform(name, outTransform, error);
}

bool BulletPhysicsService::SetTransform(const std::string& name, const btTransform& transform) {
    logging::TraceGuard trace;

    // PhysicsBridge doesn't support setting transforms in current implementation
    logging::Logger::GetInstance().Warn("SetTransform not supported by PhysicsBridge");
    return false;
}

bool BulletPhysicsService::ApplyForce(const std::string& name, const btVector3& force) {
    logging::TraceGuard trace;

    // PhysicsBridge doesn't support applying forces in current implementation
    logging::Logger::GetInstance().Warn("ApplyForce not supported by PhysicsBridge");
    return false;
}

bool BulletPhysicsService::ApplyImpulse(const std::string& name, const btVector3& impulse) {
    logging::TraceGuard trace;

    // PhysicsBridge doesn't support applying impulses in current implementation
    logging::Logger::GetInstance().Warn("ApplyImpulse not supported by PhysicsBridge");
    return false;
}

bool BulletPhysicsService::SetLinearVelocity(const std::string& name, const btVector3& velocity) {
    logging::TraceGuard trace;

    // PhysicsBridge doesn't support setting velocity in current implementation
    logging::Logger::GetInstance().Warn("SetLinearVelocity not supported by PhysicsBridge");
    return false;
}

size_t BulletPhysicsService::GetBodyCount() const {
    // PhysicsBridge doesn't expose GetBodyCount in current implementation
    // Returning 0 as stub - could track bodies in wrapper if needed
    return 0;
}

void BulletPhysicsService::Clear() {
    logging::TraceGuard trace;

    if (!physicsBridge_) {
        return;
    }

    // PhysicsBridge doesn't expose Clear in current implementation
    // Shutdown and reinitialize to clear all bodies
    physicsBridge_.reset();
    physicsBridge_ = std::make_unique<script::PhysicsBridge>();
}

}  // namespace sdl3cpp::services::impl
