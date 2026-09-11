#pragma once

#include "services/interfaces/workflow/rendering/bsp_types.hpp"

#include <assimp/mesh.h>

namespace sdl3cpp::services::impl {

/// One assimp vertex in the 40-byte position_uv_lmuv_normal layout.
BspRenderVertex MakeGta5Vertex(const aiMesh& mesh, unsigned int index);

}  // namespace sdl3cpp::services::impl
