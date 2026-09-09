#pragma once

// A tiny static Bullet world for movement tests: add boxes, walk into
// them. Shared so each test file stays focused on its assertions.

#include <btBulletDynamicsCommon.h>

#include <memory>
#include <vector>

namespace q3test {

struct Scene {
    btDefaultCollisionConfiguration config;
    btCollisionDispatcher dispatcher{&config};
    btDbvtBroadphase broadphase;
    btSequentialImpulseConstraintSolver solver;
    btDiscreteDynamicsWorld world{&dispatcher, &broadphase, &solver, &config};
    std::vector<std::unique_ptr<btBoxShape>> shapes;
    std::vector<std::unique_ptr<btDefaultMotionState>> motions;
    std::vector<std::unique_ptr<btRigidBody>> bodies;

    Scene() = default;
    Scene(const Scene&) = delete;
    Scene& operator=(const Scene&) = delete;

    /// Static box centred at `centre` with the given half extents.
    void Add(btVector3 centre, btVector3 half) {
        shapes.push_back(std::make_unique<btBoxShape>(half));
        motions.push_back(std::make_unique<btDefaultMotionState>(
            btTransform(btQuaternion(0, 0, 0, 1), centre)));
        bodies.push_back(std::make_unique<btRigidBody>(
            btRigidBody::btRigidBodyConstructionInfo(
                0.f, motions.back().get(), shapes.back().get(),
                btVector3(0, 0, 0))));
        world.addRigidBody(bodies.back().get());
    }

    /// A 100x100 floor whose top surface is y = 0.
    void AddFloor() {
        Add(btVector3(0, -10.f, 0), btVector3(50.f, 10.f, 50.f));
    }
};

}  // namespace q3test
