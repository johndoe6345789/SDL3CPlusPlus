#pragma once

#include "services/interfaces/workflow/rendering/bsp_types.hpp"

#include <assimp/scene.h>

#include <cstdint>
#include <string>
#include <vector>

namespace sdl3cpp::services::impl {

/// One material's geometry, in the 40-byte position_uv_lmuv_normal
/// layout the gta5 pipeline draws with.
///
/// The engine's own ExtractAssimpMeshData produces a 20-byte
/// position/uv vertex and drops normals, and flattens everything into a
/// single mesh. Buildings need per-vertex normals to shade, and a GTA V
/// drawable carries several textures, so it cannot be one draw.
struct Gta5SubMeshData {
    std::vector<BspRenderVertex> vertices;
    std::vector<std::uint16_t> indices;
    std::string texturePath;
};

struct Gta5MeshData {
    std::vector<Gta5SubMeshData> parts;
};

/// Split a scene into one part per assimp mesh, resolving each one's
/// diffuse texture from its material.
Gta5MeshData ExtractGta5Mesh(const aiScene& scene,
                             const std::string& baseDirectory);

}  // namespace sdl3cpp::services::impl
