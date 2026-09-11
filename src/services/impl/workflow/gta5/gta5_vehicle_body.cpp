#include "services/interfaces/workflow/gta5/gta5_vehicle_body.hpp"

#include <algorithm>
#include <limits>

namespace sdl3cpp::services::impl {
namespace {

/// Axis-aligned bounds of the collision mesh the upload already built.
bool MeshBounds(const Gta5Geometry& geometry, btVector3& centre,
                btVector3& half) {
    const auto& verts = geometry.collisionVertices;
    if (verts.empty()) return false;

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
    centre = btVector3((lo[0] + hi[0]) * 0.5f, (lo[1] + hi[1]) * 0.5f,
                       (lo[2] + hi[2]) * 0.5f);
    half = btVector3(std::max(0.1f, (hi[0] - lo[0]) * 0.5f),
                     std::max(0.1f, (hi[1] - lo[1]) * 0.5f),
                     std::max(0.1f, (hi[2] - lo[2]) * 0.5f));
    return true;
}

}  // namespace

btRigidBody* MakeGta5VehicleBody(const Gta5Geometry& geometry,
                                 const glm::vec3& position, float mass,
                                 btCollisionShape*& outShape) {
    btVector3 centre(0, 0, 0);
    btVector3 half(2.4f, 0.75f, 1.0f);
    const bool measured = MeshBounds(geometry, centre, half);

    // The body mesh is not centred on its own origin, so a box placed at
    // the origin sits lower than the car and holds it off the road. A
    // compound carries the box to where the mesh actually is.
    auto* box = new btBoxShape(half);
    auto* compound = new btCompoundShape();
    btTransform offset;
    offset.setIdentity();
    if (measured) offset.setOrigin(centre);
    compound->addChildShape(offset, box);

    btVector3 inertia(0, 0, 0);
    compound->calculateLocalInertia(mass, inertia);

    btTransform transform;
    transform.setIdentity();
    transform.setOrigin(btVector3(position.x, position.y, position.z));
    auto* motion = new btDefaultMotionState(transform);

    btRigidBody::btRigidBodyConstructionInfo info(mass, motion, compound,
                                                  inertia);
    info.m_friction = 0.9f;
    info.m_restitution = 0.05f;
    auto* body = new btRigidBody(info);
    // No manual damping: btRaycastVehicle applies suspension and tyre
    // forces itself, and damping on top of it fights the suspension.

    outShape = compound;
    return body;
}

}  // namespace sdl3cpp::services::impl
