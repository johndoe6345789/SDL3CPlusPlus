#include "physics_bridge_service.hpp"

#include <btBulletDynamicsCommon.h>

#include <utility>

namespace sdl3cpp::services::impl {

PhysicsBridgeService::PhysicsBridgeService(std::shared_ptr<ILogger> logger)
    : collisionConfig_(std::make_unique<btDefaultCollisionConfiguration>()),
      dispatcher_(std::make_unique<btCollisionDispatcher>(collisionConfig_.get())),
      broadphase_(std::make_unique<btDbvtBroadphase>()),
      solver_(std::make_unique<btSequentialImpulseConstraintSolver>()),
      world_(std::make_unique<btDiscreteDynamicsWorld>(
          dispatcher_.get(),
          broadphase_.get(),
          solver_.get(),
          collisionConfig_.get())),
      logger_(std::move(logger)) {
    if (logger_) {
        logger_->Trace("PhysicsBridgeService", "PhysicsBridgeService");
    }
    world_->setGravity(btVector3(0.0f, -9.81f, 0.0f));
}

PhysicsBridgeService::~PhysicsBridgeService() {
    if (logger_) {
        logger_->Trace("PhysicsBridgeService", "~PhysicsBridgeService");
    }
    if (world_) {
        for (auto& [name, entry] : bodies_) {
            if (entry.body) {
                world_->removeRigidBody(entry.body.get());
            }
        }
    }
}

bool PhysicsBridgeService::SetGravity(const btVector3& gravity,
                                      std::string& error) {
    if (logger_) {
        logger_->Trace("PhysicsBridgeService", "SetGravity",
                       "gravity.x=" + std::to_string(gravity.getX()) +
                       ", gravity.y=" + std::to_string(gravity.getY()) +
                       ", gravity.z=" + std::to_string(gravity.getZ()));
    }
    if (!world_) {
        error = "Physics world is not initialized";
        return false;
    }
    world_->setGravity(gravity);
    return true;
}

bool PhysicsBridgeService::AddBoxRigidBody(const std::string& name,
                                           const btVector3& halfExtents,
                                           float mass,
                                           const btTransform& transform,
                                           std::string& error) {
    if (logger_) {
        logger_->Trace("PhysicsBridgeService", "AddBoxRigidBody",
                       "name=" + name +
                       ", halfExtents.x=" + std::to_string(halfExtents.getX()) +
                       ", halfExtents.y=" + std::to_string(halfExtents.getY()) +
                       ", halfExtents.z=" + std::to_string(halfExtents.getZ()) +
                       ", mass=" + std::to_string(mass) +
                       ", origin.x=" + std::to_string(transform.getOrigin().getX()) +
                       ", origin.y=" + std::to_string(transform.getOrigin().getY()) +
                       ", origin.z=" + std::to_string(transform.getOrigin().getZ()));
    }
    if (name.empty()) {
        error = "Rigid body name must not be empty";
        return false;
    }
    if (halfExtents.getX() <= 0.0f || halfExtents.getY() <= 0.0f || halfExtents.getZ() <= 0.0f) {
        error = "Box half extents must be positive";
        return false;
    }
    if (mass < 0.0f) {
        error = "Rigid body mass must be non-negative";
        return false;
    }
    if (!world_) {
        error = "Physics world is not initialized";
        return false;
    }
    if (bodies_.count(name)) {
        error = "Rigid body already exists: " + name;
        return false;
    }
    auto shape = std::make_unique<btBoxShape>(halfExtents);
    btVector3 inertia(0.0f, 0.0f, 0.0f);
    if (mass > 0.0f) {
        shape->calculateLocalInertia(mass, inertia);
    }
    auto motionState = std::make_unique<btDefaultMotionState>(transform);
    btRigidBody::btRigidBodyConstructionInfo constructionInfo(
        mass,
        motionState.get(),
        shape.get(),
        inertia);
    auto body = std::make_unique<btRigidBody>(constructionInfo);
    world_->addRigidBody(body.get());
    bodies_.emplace(name, BodyRecord{
        std::move(shape),
        std::move(motionState),
        std::move(body),
        nullptr,
    });
    return true;
}

bool PhysicsBridgeService::AddSphereRigidBody(const std::string& name,
                                              float radius,
                                              float mass,
                                              const btTransform& transform,
                                              std::string& error) {
    if (logger_) {
        logger_->Trace("PhysicsBridgeService", "AddSphereRigidBody",
                       "name=" + name +
                       ", radius=" + std::to_string(radius) +
                       ", mass=" + std::to_string(mass) +
                       ", origin.x=" + std::to_string(transform.getOrigin().getX()) +
                       ", origin.y=" + std::to_string(transform.getOrigin().getY()) +
                       ", origin.z=" + std::to_string(transform.getOrigin().getZ()));
    }
    if (name.empty()) {
        error = "Rigid body name must not be empty";
        return false;
    }
    if (radius <= 0.0f) {
        error = "Sphere radius must be positive";
        return false;
    }
    if (mass < 0.0f) {
        error = "Rigid body mass must be non-negative";
        return false;
    }
    if (!world_) {
        error = "Physics world is not initialized";
        return false;
    }
    if (bodies_.count(name)) {
        error = "Rigid body already exists: " + name;
        return false;
    }
    auto shape = std::make_unique<btSphereShape>(radius);
    btVector3 inertia(0.0f, 0.0f, 0.0f);
    if (mass > 0.0f) {
        shape->calculateLocalInertia(mass, inertia);
    }
    auto motionState = std::make_unique<btDefaultMotionState>(transform);
    btRigidBody::btRigidBodyConstructionInfo constructionInfo(
        mass,
        motionState.get(),
        shape.get(),
        inertia);
    auto body = std::make_unique<btRigidBody>(constructionInfo);
    world_->addRigidBody(body.get());
    bodies_.emplace(name, BodyRecord{
        std::move(shape),
        std::move(motionState),
        std::move(body),
        nullptr,
    });
    return true;
}

bool PhysicsBridgeService::AddTriangleMeshRigidBody(const std::string& name,
                                                    const std::vector<std::array<float, 3>>& vertices,
                                                    const std::vector<uint32_t>& indices,
                                                    const btTransform& transform,
                                                    std::string& error) {
    if (logger_) {
        logger_->Trace("PhysicsBridgeService", "AddTriangleMeshRigidBody",
                       "name=" + name +
                       ", vertexCount=" + std::to_string(vertices.size()) +
                       ", indexCount=" + std::to_string(indices.size()) +
                       ", origin.x=" + std::to_string(transform.getOrigin().getX()) +
                       ", origin.y=" + std::to_string(transform.getOrigin().getY()) +
                       ", origin.z=" + std::to_string(transform.getOrigin().getZ()));
    }
    if (name.empty()) {
        error = "Rigid body name must not be empty";
        return false;
    }
    if (vertices.empty()) {
        error = "Triangle mesh vertices must not be empty";
        return false;
    }
    if (indices.empty()) {
        error = "Triangle mesh indices must not be empty";
        return false;
    }
    if (indices.size() % 3 != 0) {
        error = "Triangle mesh indices must be a multiple of 3";
        return false;
    }
    if (!world_) {
        error = "Physics world is not initialized";
        return false;
    }
    if (bodies_.count(name)) {
        error = "Rigid body already exists: " + name;
        return false;
    }

    auto triangleMesh = std::make_unique<btTriangleMesh>();
    for (size_t index = 0; index < indices.size(); index += 3) {
        uint32_t i0 = indices[index];
        uint32_t i1 = indices[index + 1];
        uint32_t i2 = indices[index + 2];
        if (i0 >= vertices.size() || i1 >= vertices.size() || i2 >= vertices.size()) {
            error = "Triangle mesh index out of range";
            return false;
        }
        const auto& v0 = vertices[i0];
        const auto& v1 = vertices[i1];
        const auto& v2 = vertices[i2];
        triangleMesh->addTriangle(
            btVector3(v0[0], v0[1], v0[2]),
            btVector3(v1[0], v1[1], v1[2]),
            btVector3(v2[0], v2[1], v2[2]),
            true);
    }

    auto shape = std::make_unique<btBvhTriangleMeshShape>(triangleMesh.get(), true, true);
    btVector3 inertia(0.0f, 0.0f, 0.0f);
    auto motionState = std::make_unique<btDefaultMotionState>(transform);
    btRigidBody::btRigidBodyConstructionInfo constructionInfo(
        0.0f,
        motionState.get(),
        shape.get(),
        inertia);
    auto body = std::make_unique<btRigidBody>(constructionInfo);
    world_->addRigidBody(body.get());
    bodies_.emplace(name, BodyRecord{
        std::move(shape),
        std::move(motionState),
        std::move(body),
        std::move(triangleMesh),
    });
    return true;
}

bool PhysicsBridgeService::RemoveRigidBody(const std::string& name,
                                           std::string& error) {
    if (logger_) {
        logger_->Trace("PhysicsBridgeService", "RemoveRigidBody", "name=" + name);
    }
    if (name.empty()) {
        error = "Rigid body name must not be empty";
        return false;
    }
    if (!world_) {
        error = "Physics world is not initialized";
        return false;
    }
    auto it = bodies_.find(name);
    if (it == bodies_.end()) {
        error = "Rigid body not found: " + name;
        return false;
    }
    if (it->second.body) {
        world_->removeRigidBody(it->second.body.get());
    }
    bodies_.erase(it);
    return true;
}

bool PhysicsBridgeService::SetRigidBodyTransform(const std::string& name,
                                                 const btTransform& transform,
                                                 std::string& error) {
    if (logger_) {
        logger_->Trace("PhysicsBridgeService", "SetRigidBodyTransform",
                       "name=" + name +
                       ", origin.x=" + std::to_string(transform.getOrigin().getX()) +
                       ", origin.y=" + std::to_string(transform.getOrigin().getY()) +
                       ", origin.z=" + std::to_string(transform.getOrigin().getZ()));
    }
    if (!world_) {
        error = "Physics world is not initialized";
        return false;
    }
    auto* record = FindBodyRecord(name, error);
    if (!record || !record->body) {
        return false;
    }
    record->body->setWorldTransform(transform);
    if (record->motionState) {
        record->motionState->setWorldTransform(transform);
    }
    record->body->activate(true);
    return true;
}

bool PhysicsBridgeService::ApplyForce(const std::string& name,
                                      const btVector3& force,
                                      std::string& error) {
    if (logger_) {
        logger_->Trace("PhysicsBridgeService", "ApplyForce",
                       "name=" + name +
                       ", force.x=" + std::to_string(force.getX()) +
                       ", force.y=" + std::to_string(force.getY()) +
                       ", force.z=" + std::to_string(force.getZ()));
    }
    if (!world_) {
        error = "Physics world is not initialized";
        return false;
    }
    auto* record = FindBodyRecord(name, error);
    if (!record || !record->body) {
        return false;
    }
    record->body->applyCentralForce(force);
    record->body->activate(true);
    return true;
}

bool PhysicsBridgeService::ApplyImpulse(const std::string& name,
                                        const btVector3& impulse,
                                        std::string& error) {
    if (logger_) {
        logger_->Trace("PhysicsBridgeService", "ApplyImpulse",
                       "name=" + name +
                       ", impulse.x=" + std::to_string(impulse.getX()) +
                       ", impulse.y=" + std::to_string(impulse.getY()) +
                       ", impulse.z=" + std::to_string(impulse.getZ()));
    }
    if (!world_) {
        error = "Physics world is not initialized";
        return false;
    }
    auto* record = FindBodyRecord(name, error);
    if (!record || !record->body) {
        return false;
    }
    record->body->applyCentralImpulse(impulse);
    record->body->activate(true);
    return true;
}

bool PhysicsBridgeService::SetLinearVelocity(const std::string& name,
                                             const btVector3& velocity,
                                             std::string& error) {
    if (logger_) {
        logger_->Trace("PhysicsBridgeService", "SetLinearVelocity",
                       "name=" + name +
                       ", velocity.x=" + std::to_string(velocity.getX()) +
                       ", velocity.y=" + std::to_string(velocity.getY()) +
                       ", velocity.z=" + std::to_string(velocity.getZ()));
    }
    if (!world_) {
        error = "Physics world is not initialized";
        return false;
    }
    auto* record = FindBodyRecord(name, error);
    if (!record || !record->body) {
        return false;
    }
    record->body->setLinearVelocity(velocity);
    record->body->activate(true);
    return true;
}

bool PhysicsBridgeService::GetLinearVelocity(const std::string& name,
                                             btVector3& outVelocity,
                                             std::string& error) const {
    if (logger_) {
        logger_->Trace("PhysicsBridgeService", "GetLinearVelocity", "name=" + name);
    }
    if (!world_) {
        error = "Physics world is not initialized";
        return false;
    }
    auto* record = FindBodyRecord(name, error);
    if (!record || !record->body) {
        return false;
    }
    outVelocity = record->body->getLinearVelocity();
    return true;
}
int PhysicsBridgeService::StepSimulation(float deltaTime, int maxSubSteps) {
    if (logger_) {
        logger_->Trace("PhysicsBridgeService", "StepSimulation",
                       "deltaTime=" + std::to_string(deltaTime) +
                       ", maxSubSteps=" + std::to_string(maxSubSteps));
    }
    if (!world_) {
        return 0;
    }
    if (maxSubSteps < 0) {
        maxSubSteps = 0;
    }
    return world_->stepSimulation(deltaTime, maxSubSteps, 1.0f / 60.0f);
}

bool PhysicsBridgeService::GetRigidBodyTransform(const std::string& name,
                                                 btTransform& outTransform,
                                                 std::string& error) const {
    if (logger_) {
        logger_->Trace("PhysicsBridgeService", "GetRigidBodyTransform", "name=" + name);
    }
    auto* record = FindBodyRecord(name, error);
    if (!record) {
        return false;
    }
    if (record->motionState) {
        record->motionState->getWorldTransform(outTransform);
        return true;
    }
    if (record->body) {
        outTransform = record->body->getWorldTransform();
        return true;
    }
    error = "Rigid body handle is missing";
    return false;
}

size_t PhysicsBridgeService::GetBodyCount() const {
    if (logger_) {
        logger_->Trace("PhysicsBridgeService", "GetBodyCount");
    }
    return bodies_.size();
}

void PhysicsBridgeService::Clear() {
    if (logger_) {
        logger_->Trace("PhysicsBridgeService", "Clear");
    }
    if (world_) {
        for (auto& [name, entry] : bodies_) {
            if (entry.body) {
                world_->removeRigidBody(entry.body.get());
            }
        }
    }
    bodies_.clear();
}

PhysicsBridgeService::BodyRecord* PhysicsBridgeService::FindBodyRecord(const std::string& name,
                                                                       std::string& error) {
    auto it = bodies_.find(name);
    if (it == bodies_.end()) {
        error = "Rigid body not found: " + name;
        return nullptr;
    }
    if (!it->second.body) {
        error = "Rigid body handle is missing";
        return nullptr;
    }
    return &it->second;
}

const PhysicsBridgeService::BodyRecord* PhysicsBridgeService::FindBodyRecord(const std::string& name,
                                                                             std::string& error) const {
    auto it = bodies_.find(name);
    if (it == bodies_.end()) {
        error = "Rigid body not found: " + name;
        return nullptr;
    }
    if (!it->second.body) {
        error = "Rigid body handle is missing";
        return nullptr;
    }
    return &it->second;
}

}  // namespace sdl3cpp::services::impl
