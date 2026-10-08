#pragma once

#include "services/interfaces/workflow/geometry/geometry_plane_helpers.hpp"

namespace sdl3cpp::services::impl {

/// A 3D arrow about 4.5 m long, pointing down +Z. Each face's shade is
/// carried in uv.x for the arrow fragment shader.
GeometryPlaneMesh BuildSwitchbackArrowMesh();

}  // namespace sdl3cpp::services::impl
