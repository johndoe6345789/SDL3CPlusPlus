#pragma once

#include "services/interfaces/workflow/gta5/gta5_asset_index.hpp"
#include "services/interfaces/workflow/gta5/gta5_geometry.hpp"
#include "services/interfaces/workflow/gta5/gta5_mesh_extract.hpp"
#include "services/interfaces/workflow/gta5/gta5_resource_cache.hpp"
#include "services/interfaces/workflow/gta5/gta5_texture_upload.hpp"

#include <cstdint>
#include <mutex>
#include <string>
#include <unordered_set>
#include <vector>

namespace sdl3cpp::services::impl {

/// Which textures a job has taken on reading, so each is read once
/// however many archetypes name it.
class Gta5TextureClaims {
public:
    /// True for the first caller only.
    bool Claim(std::uint32_t hash);

private:
    std::mutex lock_;
    std::unordered_set<std::uint32_t> claimed_;
};

/// Everything CPU-side for one archetype, done on a worker thread: the
/// drawable read and decoded, its collision BVH built -- the costliest
/// step -- and the pixels of each texture it names that no other job has
/// claimed. The main thread only uploads it.
struct Gta5PreparedGeometry {
    std::string key;
    Gta5MeshData mesh;
    /// Every texture its parts use; it is drawable once all are resolved.
    std::vector<std::uint32_t> textures;
    /// The ones this job read.
    std::vector<Gta5TextureBlob> blobs;
    /// Only the collision fields are used. The BVH points into these
    /// arrays, and moving a vector keeps its buffer, so they move over.
    Gta5Geometry collision;
};

/// Prepare one archetype. Thread-safe: it reads the index, which does not
/// change once built, and the resource cache, which locks.
Gta5PreparedGeometry PrepareGta5Geometry(const Gta5AssetIndex& index,
                                         Gta5ResourceCache& resources,
                                         Gta5TextureClaims& claims,
                                         const std::string& key,
                                         std::uint32_t hash);

}  // namespace sdl3cpp::services::impl
