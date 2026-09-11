#include "services/interfaces/workflow/gta5/gta5_vehicle_body.hpp"

#include <algorithm>
#include <limits>

namespace sdl3cpp::services::impl {
namespace {

/// Half extents of the collision mesh the upload already built.
btVector3 HalfExtents(const Gta5Geometry& geometry) {
    const auto& verts = geometry.collisionVertices;
    if (verts.empty()) return btVector3(2.4f, 0.75f, 1.0f);

    float lo[3] = {std::numeric_limits<float>::max(),
                   std::numeric_limits<float>::max(),
                   std::numeric_limits<float>::max()};
    float hi[3] = {-lo[0], -lo[0], -lo[0]};
    for (std::size_t i = 0; i + 2 < verts.size(); i += 3) {
        for (int axis = 0; axis < 3; ++axis) {
            const auto value = static_cast<float>(verts[i + axis]);
            lo[axis] = std::min(lo[axis], value);
            hi[axis] = std::max(hi[axis], value);
        }
    }
    return btVector3(std::max(0.1f, (hi[0] - lo[0]) * 0.5f),
                     std::max(0.1f, (hi[1] - lo[1]) * 0.5f),
                     std::max(0.1f, (hi[2] - lo[2]) * 0.5f));
}

}  // namespace

btRigidBody* MakeGta5VehicleBody(const Gta5Geometry& geometry,
                                 const glm::vec3& position, float mass,
                                 btCollisionShape*& outShape) {
    auto* shape = new btBoxShape(HalfExtents(geometry));
    btVector3 inertia(0, 0, 0);
    shape->calculateLocalInertia(mass, inertia);

    btTransform transform;
    transform.setIdentity();
    transform.setOrigin(btVector3(position.x, position.y, position.z));
    auto* motion = new btDefaultMotionState(transform);

    btRigidBody::btRigidBodyConstructionInfo info(mass, motion, shape,
                                                  inertia);
    info.m_friction = 0.9f;
    info.m_restitution = 0.05f;
    auto* body = new btRigidBody(info);
    // Cars settle; without damping the box skates along the road.
    body->setDamping(0.15f, 0.6f);

    outShape = shape;
    return body;
}

}  // namespace sdl3cpp::services::impl
