#pragma once

#include "services/interfaces/workflow/rendering/bsp_types.hpp"

#include <assimp/scene.h>

#include <cstdint>
#include <string>
#include <vector>

namespace sdl3cpp::services::impl {

/// One OBJ "g" group's geometry (bl4x writes one per material index --
/// see cmd_bake_region in bl4x's main.cpp), in the 40-byte
/// position_uv_lmuv_normal layout the rest of the engine already draws
/// with. bl4x doesn't resolve UMaterialInstance textures yet, so there
/// is no texture path here: bl4.models.draw binds one placeholder
/// texture for every submesh (see packages/bl4/shaders/spirv/bl4_model.frag).
struct Bl4SubMeshData {
    std::vector<BspRenderVertex> vertices;
    std::vector<std::uint32_t> indices;
};

struct Bl4MeshData {
    std::vector<Bl4SubMeshData> parts;
};

/// Split a scene into one part per assimp mesh. `aiProcess_Triangulate |
/// aiProcess_GenNormals | aiProcess_JoinIdenticalVertices` is the
/// expected import flag set (see bl4_tiles_load_step.cpp).
Bl4MeshData ExtractBl4Mesh(const aiScene& scene);

}  // namespace sdl3cpp::services::impl
