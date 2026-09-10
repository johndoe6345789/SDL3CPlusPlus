#pragma once

#include "services/interfaces/workflow/rendering/bsp_types.hpp"

#include <assimp/scene.h>

#include <cstdint>
#include <vector>

namespace sdl3cpp::services::impl {

/// An archetype's mesh on the CPU, in the 40-byte
/// `position_uv_lmuv_normal` layout the gta5 pipeline draws with.
///
/// The engine's own ExtractAssimpMeshData drops normals -- it produces
/// the 20-byte position/uv vertex -- and without them the shader has to
/// light every surface from one constant direction. Buildings need real
/// normals, so this extracts them.
struct Gta5MeshData {
    std::vector<BspRenderVertex> vertices;
    std::vector<std::uint16_t> indices;
};

/// Flatten every mesh in a scene into one vertex/index pair.
Gta5MeshData ExtractGta5Mesh(const aiScene& scene);

}  // namespace sdl3cpp::services::impl
