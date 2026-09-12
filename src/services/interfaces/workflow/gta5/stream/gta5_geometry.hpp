#pragma once

#include "services/interfaces/workflow/gta5/stream/gta5_geometry_arena.hpp"

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
/// and its texture, borrowed from the texture cache. Several a drawable.
struct Gta5SubMesh {
    Gta5ArenaSlot slot;
    std::uint32_t indexCount{0};
    SDL_GPUTexture* texture{nullptr};
    SDL_GPUSampler* sampler{nullptr};
    std::array<std::uint32_t, 2> textureSize{};  // width, height, for F3
    /// rgb tints the texture, a is the alpha-discard threshold: one vec4.
    std::array<float, 4> surface{1.f, 1.f, 1.f, 0.f};
    bool blend{false};  // alpha-blended, drawn after the opaque scene
    bool terrain{false};  // four layers; see gta5_terrain.frag
    bool emissive{false};  // windows, signs: see gta5_emissive.frag
    bool paint{false};  // vehicle_paint: what a respray recolours
    std::array<SDL_GPUTexture*, 5> layers{};  // 4 layers, then the mask
    std::array<SDL_GPUSampler*, 5> layerSamplers{};
    int DrawKind() const {
        return blend ? 4 : emissive ? 3 : terrain ? 2 : surface[3] > 0.f;
    }
};

/// An archetype's mesh, uploaded once and drawn many times.
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
    /// Collision mesh, every submesh merged, shared at unit scale. Bullet
    /// keeps pointers into these arrays, so they live here, beside it.
    std::vector<btScalar> collisionVertices;
    std::vector<int> collisionIndices;
    btTriangleIndexVertexArray* collisionMesh{nullptr};
    btBvhTriangleMeshShape* collisionShape{nullptr};
};

/// One placed copy of an archetype. Its geometry pointer addresses an
/// element of the cache's unordered_map, whose element addresses are
/// stable across rehashing, so it stays valid as more are cached.
struct Gta5Instance {
    Gta5Geometry* geometry{nullptr};
    std::array<float, 16> modelMatrix{};
    btRigidBody* body{nullptr};
    /// Its own scaled wrapper of the shared shape, off unit scale only.
    btCollisionShape* scaledShape{nullptr};
    /// Drawn from childLodDist out to lodDist, as its placement says.
    float lodDist{0.f};
    float childLodDist{0.f};
    std::uint32_t archetype{0};  // name hash, for F3
    std::uint8_t proxy{0};  // a Gta5ProxyKind: 0 is scenery
};

}  // namespace sdl3cpp::services::impl
