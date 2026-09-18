#pragma once

#include "services/interfaces/workflow/rendering/bsp_types.hpp"

#include <assimp/scene.h>

#include <cstdint>
#include <string>
#include <vector>

namespace sdl3cpp::services::impl {

/// One OBJ material group's geometry (bl4x writes one per material
/// slot -- see cmd_bake_region in bl4x's main.cpp), in the 40-byte
/// position_uv_lmuv_normal layout the rest of the engine already draws
/// with, plus the base-colour map its .mtl names (map_Kd, relative to
/// the OBJ; empty when bl4x found none for that material).
struct Bl4SubMeshData {
    std::vector<BspRenderVertex> vertices;
    std::vector<std::uint32_t> indices;
    std::string texturePath;
};

struct Bl4MeshData {
    std::vector<Bl4SubMeshData> parts;
    /// Bounding sphere of every part together, in model space; radius
    /// is negative when the model held no vertices.
    float boundsCenter[3] = {0.f, 0.f, 0.f};
    float boundsRadius = -1.f;
};

/// Split a scene into one part per assimp mesh. `aiProcess_Triangulate |
/// aiProcess_GenNormals | aiProcess_JoinIdenticalVertices` is the
/// expected import flag set (see bl4_tiles_load_step.cpp).
Bl4MeshData ExtractBl4Mesh(const aiScene& scene);

}  // namespace sdl3cpp::services::impl
