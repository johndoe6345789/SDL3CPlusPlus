#pragma once

#include "services/interfaces/workflow/geometry/geometry_plane_helpers.hpp"

#include <glm/glm.hpp>

namespace sdl3cpp::services::impl {

/// The Z every dashboard vertex sits at: nearest the camera, drawn
/// with an identity MVP so (x, y) are screen-space NDC directly.
constexpr float kStuntsDashboardDepth = 0.f;

/// The palette texture's U coordinate for solid colour `materialId`.
float StuntsDashboardPaletteU(int materialId);

/// A flat quad from four corners, in winding order, one solid colour.
void AppendStuntsDashboardQuad(GeometryPlaneMesh& mesh, glm::vec2 a,
                              glm::vec2 b, glm::vec2 c, glm::vec2 d,
                              float paletteU);

/// A filled disc, fan-triangulated from its centre.
void AppendStuntsDashboardDisc(GeometryPlaneMesh& mesh, glm::vec2 centre,
                              float rx, float ry, int materialId);

/// A wheel rim: an annulus of trapezoid segments.
void AppendStuntsWheelRim(GeometryPlaneMesh& mesh, glm::vec2 centre,
                         float rIn, float rOut, int materialId);

}  // namespace sdl3cpp::services::impl
