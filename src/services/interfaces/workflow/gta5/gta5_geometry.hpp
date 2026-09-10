#pragma once

#include <SDL3/SDL_gpu.h>
#include <btBulletDynamicsCommon.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace sdl3cpp::services::impl {

/// Index buffers are 16-bit, so one archetype cannot exceed this many
/// vertices. Some GTA V building drawables do.
inline constexpr std::size_t kGta5MaxVerticesPerMesh = 65536u;

/// An archetype's mesh, uploaded once and drawn many times.
///
/// references counts the live instances pointing at it. Both the GPU
/// buffers and the collision shape are released when that reaches zero,
/// so walking out of a district frees its memory rather than holding
/// every archetype ever seen.
struct Gta5Geometry {
    SDL_GPUBuffer* vertexBuffer{nullptr};
    SDL_GPUBuffer* indexBuffer{nullptr};
    std::uint32_t indexCount{0};
    int references{0};
    bool usable{false};

    /// Collision mesh, shared by every instance of this archetype at unit
    /// scale. Bullet does not copy these arrays, so they have to outlive
    /// the shape that indexes them: they live here, beside it.
    std::vector<btScalar> collisionVertices;
    std::vector<int> collisionIndices;
    btTriangleIndexVertexArray* collisionMesh{nullptr};
    btBvhTriangleMeshShape* collisionShape{nullptr};
};

/// One placed copy of an archetype.
///
/// The geometry pointer addresses an element of the cache's
/// unordered_map, whose element addresses are stable across rehashing,
/// so it stays valid as more archetypes are cached.
struct Gta5Instance {
    Gta5Geometry* geometry{nullptr};
    std::array<float, 16> modelMatrix{};
    /// Static body in the physics world, so the player can stand on it.
    btRigidBody* body{nullptr};
    /// Non-null only when this instance is not at unit scale and needed
    /// its own scaled wrapper around the shared collision shape.
    btCollisionShape* scaledShape{nullptr};
};

}  // namespace sdl3cpp::services::impl
