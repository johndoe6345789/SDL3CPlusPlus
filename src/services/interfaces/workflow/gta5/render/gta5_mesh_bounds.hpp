#pragma once

#include "services/interfaces/workflow/gta5/render/gta5_mesh_extract.hpp"

#include <array>

namespace sdl3cpp::services::impl {

/// A sphere about the mesh's box centre, reaching its furthest vertex:
/// centre xyz, radius w; a negative radius when the mesh is empty.
std::array<float, 4> Gta5MeshBounds(const Gta5MeshData& mesh);

}  // namespace sdl3cpp::services::impl
