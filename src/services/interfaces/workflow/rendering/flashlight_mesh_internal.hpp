#pragma once

/// Internal primitive builders shared by flashlight_mesh_primitives.cpp
/// and flashlight_mesh_build.cpp, which together implement
/// BuildFlashlightMesh() from flashlight_mesh.hpp. Not part of the public
/// workflow-step API.

#include "services/interfaces/workflow/rendering/flashlight_mesh.hpp"

namespace sdl3cpp::services::impl::flashlight_mesh_detail {

/// Adds a cylinder section (or cone frustum, when r1 != r2) as a ring of
/// vertices at each end and two triangles per segment between them.
void AddCylinder(std::vector<PosUvVertex>& vertices,
                 std::vector<uint16_t>& indices, int segments, float r1,
                 float r2, float y_start, float y_end, float uv_start,
                 float uv_end);

/// Adds a disc cap (a center vertex fanned out to a ring). `flip` reverses
/// winding order for a cap that faces -Y instead of +Y.
void AddCap(std::vector<PosUvVertex>& vertices, std::vector<uint16_t>& indices,
            int segments, float radius, float y, float uv_v, bool flip);

}  // namespace sdl3cpp::services::impl::flashlight_mesh_detail
