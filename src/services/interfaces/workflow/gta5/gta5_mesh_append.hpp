#pragma once

#include "services/interfaces/workflow/gta5/gta5_placement.hpp"

#include <assimp/scene.h>

#include <cstddef>

namespace sdl3cpp::services::impl {

/// SceneObject indexes with uint16_t, so one object cannot hold more than
/// this many vertices. GTA V building drawables regularly exceed it.
inline constexpr std::size_t kGta5MaxVerticesPerObject = 65536u;

/// Append one assimp mesh to a geometry, offsetting its indices.
void AppendGta5Mesh(const aiMesh& mesh, Gta5Geometry& geometry);

/// Total vertices across every mesh in a scene.
std::size_t CountGta5SceneVertices(const aiScene& scene);

}  // namespace sdl3cpp::services::impl
