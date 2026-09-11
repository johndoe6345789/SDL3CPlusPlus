#pragma once

#include "services/interfaces/workflow/rendering/bsp_types.hpp"

#include <assimp/scene.h>

#include <array>
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
    /// Diffuse texture by name hash, for geometry read straight from a
    /// drawable; resolved through the asset index. 0 when not used.
    std::uint32_t textureHash{0};
    /// Multiplies the texture. GTA V paint textures are a few white
    /// pixels with the colour supplied per vehicle, so without this
    /// every car renders white.
    std::array<float, 3> tint{1.f, 1.f, 1.f};
    /// Discard below this alpha; 0 draws everything. Foliage is a
    /// rectangle whose shape is entirely in its alpha.
    float alphaCutoff{0.f};
    /// Drawn blended over the opaque scene: decals, glass.
    bool blend{false};
    /// Terrain: four diffuse layers, weighted by vertex colour 1.
    bool terrain{false};
    std::array<std::uint32_t, 5> layerHashes{};  // then the mask
};

struct Gta5MeshData {
    std::vector<Gta5SubMeshData> parts;
};

/// Split a scene into one part per assimp mesh, resolving each one's
/// diffuse texture from its material.
Gta5MeshData ExtractGta5Mesh(const aiScene& scene,
                             const std::string& baseDirectory);

}  // namespace sdl3cpp::services::impl
