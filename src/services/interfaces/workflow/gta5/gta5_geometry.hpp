#pragma once

#include <SDL3/SDL_gpu.h>
#include <btBulletDynamicsCommon.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace sdl3cpp::services::impl {

/// Index buffers are 16-bit, so one submesh cannot exceed this many
/// vertices. Splitting a drawable per material keeps most well under it.
inline constexpr std::size_t kGta5MaxVerticesPerMesh = 65536u;

/// One material's worth of an archetype: its own buffers and texture.
///
/// A GTA V drawable is several geometries with different textures, so it
/// cannot be one draw. The texture and sampler are borrowed from the
/// state's texture cache and are not owned here.
struct Gta5SubMesh {
    SDL_GPUBuffer* vertexBuffer{nullptr};
    SDL_GPUBuffer* indexBuffer{nullptr};
    std::uint32_t indexCount{0};
    SDL_GPUTexture* texture{nullptr};
    SDL_GPUSampler* sampler{nullptr};
    /// rgb multiplies the texture, a is the alpha-discard threshold.
    /// The draw pushes this as one vec4, so they travel together.
    std::array<float, 4> surface{1.f, 1.f, 1.f, 0.f};
};

/// An archetype's mesh, uploaded once and drawn many times.
///
/// references counts the live instances pointing at it. The buffers and
/// the collision shape are released when that reaches zero, so walking
/// out of a district frees its memory rather than holding every
/// archetype ever seen.
struct Gta5Geometry {
    std::vector<Gta5SubMesh> subMeshes;
    int references{0};
    bool usable{false};

    /// Collision mesh, every submesh merged, shared by each instance at
    /// unit scale. Bullet does not copy these arrays, so they have to
    /// outlive the shape indexing them: they live here, beside it.
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
    btRigidBody* body{nullptr};
    /// Non-null only when this instance is not at unit scale and needed
    /// its own scaled wrapper around the shared collision shape.
    btCollisionShape* scaledShape{nullptr};
};

}  // namespace sdl3cpp::services::impl
