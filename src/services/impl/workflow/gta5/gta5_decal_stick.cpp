#include "services/interfaces/workflow/gta5/gta5_decal_stick.hpp"

#include <btBulletDynamicsCommon.h>

#include <cmath>

namespace sdl3cpp::services::impl {
namespace {

glm::vec3 Corner(const Gta5Geometry& mesh, int index) {
    const std::size_t at = 3 * static_cast<std::size_t>(index);
    return glm::vec3(static_cast<float>(mesh.collisionVertices[at]),
                     static_cast<float>(mesh.collisionVertices[at + 1]),
                     static_cast<float>(mesh.collisionVertices[at + 2]));
}

/// Moller-Trumbore: how far along `from` + t * `along` the segment
/// crosses the triangle, or a negative when it misses. `along` is not a
/// unit, so t runs 0 to 1 across the whole shot.
float Crosses(const glm::vec3& from, const glm::vec3& along,
              const glm::vec3& a, const glm::vec3& b, const glm::vec3& c) {
    const glm::vec3 ab = b - a, ac = c - a;
    const glm::vec3 across = glm::cross(along, ac);
    const float det = glm::dot(ab, across);
    if (std::fabs(det) < 1e-9f) return -1.f;  // edge on
    const float share = 1.f / det;
    const glm::vec3 out = from - a;
    const float u = share * glm::dot(out, across);
    if (u < 0.f || u > 1.f) return -1.f;
    const glm::vec3 up = glm::cross(out, ab);
    const float v = share * glm::dot(along, up);
    if (v < 0.f || u + v > 1.f) return -1.f;
    return share * glm::dot(ac, up);
}

}  // namespace

bool Gta5StickToCar(const Gta5Vehicle& car, const glm::vec3& from,
                    const glm::vec3& to, Gta5Stuck& stuck) {
    const Gta5Geometry* mesh = car.instance.geometry;
    if (!car.chassis || !mesh || mesh->collisionIndices.empty()) return false;
    // Into the car's own space, where its triangles are.
    const btTransform back = car.chassis->getWorldTransform().inverse();
    const btVector3 a = back * btVector3(from.x, from.y, from.z);
    const btVector3 b = back * btVector3(to.x, to.y, to.z);
    const glm::vec3 start(a.x(), a.y(), a.z());
    const glm::vec3 along = glm::vec3(b.x(), b.y(), b.z()) - start;
    const auto& index = mesh->collisionIndices;
    float nearest = 1.f;
    glm::vec3 facing(0.f);
    for (std::size_t i = 0; i + 2 < index.size(); i += 3) {
        const glm::vec3 p = Corner(*mesh, index[i]);
        const glm::vec3 q = Corner(*mesh, index[i + 1]);
        const glm::vec3 r = Corner(*mesh, index[i + 2]);
        const float t = Crosses(start, along, p, q, r);
        if (t < 0.f || t >= nearest) continue;
        const glm::vec3 out = glm::cross(q - p, r - p);
        if (glm::length(out) < 1e-12f) continue;
        nearest = t;
        // Out of the panel, towards whatever the shot came from.
        facing = glm::normalize(out);
        if (glm::dot(facing, along) > 0.f) facing = -facing;
    }
    if (facing == glm::vec3(0.f)) return false;
    stuck.on = car.chassis;
    // A finger's width clear of the paint, so it does not z-fight it.
    stuck.local = start + along * nearest + facing * 0.01f;
    stuck.normal = facing;
    return true;
}

glm::vec3 Gta5StuckAt(const Gta5Stuck& stuck) {
    if (!stuck.on) return stuck.local;
    const btVector3 at = stuck.on->getWorldTransform() *
                         btVector3(stuck.local.x, stuck.local.y,
                                   stuck.local.z);
    return glm::vec3(at.x(), at.y(), at.z());
}

}  // namespace sdl3cpp::services::impl
