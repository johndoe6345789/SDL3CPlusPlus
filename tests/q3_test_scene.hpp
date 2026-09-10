#pragma once

// A tiny static Bullet world for movement tests: add boxes, walk into
// them. Shared so each test file stays focused on its assertions.

#include "services/interfaces/workflow/quake3/q3_brush_collision.hpp"

#include <btBulletDynamicsCommon.h>

#include <algorithm>
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

    /// The same static geometry as brush planes. Pmove traces this, not
    /// the Bullet shapes, so the movement tests exercise what the game
    /// runs; the world carries it exactly as bsp.build_collision does.
    sdl3cpp::services::impl::BrushCollisionModel brushes;

    Scene() { world.setWorldUserInfo(&brushes); }
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
        AddBrush(btTransform(btQuaternion(0, 0, 0, 1), centre), half);
    }

    /// Static box rotated about +Z, so its top face slopes along X.
    /// A ramp is the shape a slide move has to climb rather than stop
    /// against, which a stack of axis-aligned boxes does not exercise.
    void AddRamp(btVector3 centre, btVector3 half, float pitchRadians) {
        shapes.push_back(std::make_unique<btBoxShape>(half));
        btQuaternion rotation(btVector3(0.f, 0.f, 1.f), pitchRadians);
        motions.push_back(std::make_unique<btDefaultMotionState>(
            btTransform(rotation, centre)));
        bodies.push_back(std::make_unique<btRigidBody>(
            btRigidBody::btRigidBodyConstructionInfo(
                0.f, motions.back().get(), shapes.back().get(),
                btVector3(0, 0, 0))));
        world.addRigidBody(bodies.back().get());
        AddBrush(btTransform(rotation, centre), half);
    }

    /// The six planes of a (possibly rotated) box, as q3map2 would emit
    /// them. A rotated box's planes are its axes, so the bevels a box
    /// trace needs are present either way.
    void AddBrush(const btTransform& transform, const btVector3& half) {
        namespace impl = sdl3cpp::services::impl;
        impl::CollisionBrush brush;
        brush.firstSide = static_cast<int>(brushes.sides.size());
        brush.numSides  = 6;
        brush.mins = glm::vec3(1e30f);
        brush.maxs = glm::vec3(-1e30f);

        for (int axis = 0; axis < 3; ++axis) {
            for (int sign = -1; sign <= 1; sign += 2) {
                btVector3 local(0, 0, 0);
                local[axis] = static_cast<btScalar>(sign);
                const btVector3 n = transform.getBasis() * local;
                const btVector3 onFace =
                    transform * (local * half[axis]);
                impl::BrushPlane plane;
                plane.normal = glm::vec3(n.x(), n.y(), n.z());
                plane.dist   = static_cast<float>(n.dot(onFace));
                brushes.sides.push_back(plane);
            }
        }
        // Bounds from the transformed corners, since a rotated box's
        // extent is not its half extents.
        for (int corner = 0; corner < 8; ++corner) {
            const btVector3 local((corner & 1) ? half.x() : -half.x(),
                                  (corner & 2) ? half.y() : -half.y(),
                                  (corner & 4) ? half.z() : -half.z());
            const btVector3 p = transform * local;
            for (int axis = 0; axis < 3; ++axis) {
                brush.mins[axis] = std::min(brush.mins[axis], float(p[axis]));
                brush.maxs[axis] = std::max(brush.maxs[axis], float(p[axis]));
            }
        }
        brushes.brushes.push_back(brush);
    }

    /// A 100x100 floor whose top surface is y = 0.
    void AddFloor() {
        Add(btVector3(0, -10.f, 0), btVector3(50.f, 10.f, 50.f));
    }

    /// The player's own capsule, as physics.body.add creates it: a
    /// dynamic body of mass 80, not a static one. The distinction
    /// matters — Bullet's sweep reports a dynamic body it starts inside
    /// but not a static one, so a harness that uses mass 0 cannot see a
    /// trace failing to exclude the player.
    btRigidBody* AddPlayerBody(btVector3 at) {
        shapes.push_back(std::make_unique<btBoxShape>(
            btVector3(0.3f, 0.5f, 0.3f)));
        motions.push_back(std::make_unique<btDefaultMotionState>(
            btTransform(btQuaternion(0, 0, 0, 1), at)));
        btVector3 inertia(0, 0, 0);
        shapes.back()->calculateLocalInertia(80.f, inertia);
        bodies.push_back(std::make_unique<btRigidBody>(
            btRigidBody::btRigidBodyConstructionInfo(
                80.f, motions.back().get(), shapes.back().get(), inertia)));
        world.addRigidBody(bodies.back().get());
        return bodies.back().get();
    }
};

}  // namespace q3test
