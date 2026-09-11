#pragma once

#include "services/interfaces/workflow/gta5/gta5_geometry_arena.hpp"

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

/// One material's worth of an archetype: its range of the geometry arena
/// and its texture -- borrowed from the texture cache, not owned. A GTA V
/// drawable is several geometries with different textures: several draws.
struct Gta5SubMesh {
    Gta5ArenaSlot slot;
    std::uint32_t indexCount{0};
    SDL_GPUTexture* texture{nullptr};
    SDL_GPUSampler* sampler{nullptr};
    /// rgb tints the texture, a is the alpha-discard threshold: one vec4.
    std::array<float, 4> surface{1.f, 1.f, 1.f, 0.f};
    bool blend{false};  // alpha-blended, drawn after the opaque scene
    bool terrain{false};  // four layers; see gta5_terrain.frag
    std::array<SDL_GPUTexture*, 4> layers{};
    std::array<SDL_GPUSampler*, 4> layerSamplers{};
    int DrawKind() const { return blend ? 2 : (terrain ? 1 : 0); }  // order
};

/// An archetype's mesh, uploaded once and drawn many times.
///
/// references counts the live instances pointing at it. The buffers and
/// the collision shape are released when that reaches zero, so walking
/// out of a district frees its memory rather than holding every
/// archetype ever seen.
struct Gta5Geometry {
    std::vector<Gta5SubMesh> subMeshes;
    /// Bounding sphere in model space: centre xyz, radius w. A negative
    /// radius is unknown, and such geometry is never culled.
    std::array<float, 4> bounds{0.f, 0.f, 0.f, -1.f};
    int references{0};
    bool usable{false};
    /// Being prepared by the load pool; drawable once it lands.
    bool pending{false};
    /// Tried, and found to have nothing to draw; not asked for again.
    bool failed{false};

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
    /// Drawn from childLodDist out to lodDist, as its placement says.
    float lodDist{0.f};
    float childLodDist{0.f};
};

}  // namespace sdl3cpp::services::impl
