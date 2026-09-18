#pragma once

#include <SDL3/SDL_gpu.h>
#include <btBulletDynamicsCommon.h>
#include <glm/glm.hpp>

#include <cstdint>
#include <string>
#include <vector>

namespace sdl3cpp::services::impl {

/// One assimp mesh's worth of an archetype's geometry: its own GPU
/// vertex/index buffers and base-colour map (bl4x writes one OBJ group
/// per material slot -- see Bl4MeshData).
struct Bl4SubMesh {
    SDL_GPUBuffer* vertexBuffer = nullptr;
    SDL_GPUBuffer* indexBuffer = nullptr;
    std::uint32_t indexCount = 0;
    /// Key into Bl4TileStreamState::textureCache; empty, with a null
    /// texture, when the material had no map (drawn with the placeholder).
    std::string texturePath;
    SDL_GPUTexture* texture = nullptr;
    SDL_GPUSampler* sampler = nullptr;
};

/// An archetype's mesh, uploaded once and drawn many times. references
/// counts the live instances pointing at it; the buffers and collision
/// shape are released when that reaches zero, so walking out of a
/// region frees its memory rather than holding every mesh ever seen
/// (mirrors packages/gta5's Gta5Geometry).
struct Bl4Geometry {
    std::string modelPath;  // this cache entry's key, for release-on-zero-refs
    std::vector<Bl4SubMesh> subMeshes;
    int references = 0;
    bool usable = false;
    /// Collision mesh, every submesh merged, at unit scale. Bullet keeps
    /// pointers into these arrays, so they live here, beside it.
    std::vector<btScalar> collisionVertices;
    std::vector<int> collisionIndices;
    btTriangleIndexVertexArray* collisionMesh = nullptr;
    btBvhTriangleMeshShape* collisionShape = nullptr;
    /// Model-space bounding sphere, for the draw step's culling.
    glm::vec3 boundsCenter{0.f};
    float boundsRadius = -1.f;
};

/// One placed copy of an archetype.
struct Bl4Instance {
    Bl4Geometry* geometry = nullptr;
    glm::mat4 modelMatrix{1.f};
    btRigidBody* body = nullptr;
    /// Its own scaled wrapper of the shared shape, off unit scale only.
    btCollisionShape* scaledShape = nullptr;
    /// This placement's bounding sphere in world space, so culling costs
    /// no matrix work per frame.
    glm::vec3 boundsCenter{0.f};
    float boundsRadius = -1.f;
};

}  // namespace sdl3cpp::services::impl
