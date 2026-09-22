#pragma once

#include "services/interfaces/workflow/geometry/geometry_plane_helpers.hpp"
#include "services/interfaces/workflow/stunts/data/stunts_shape.hpp"

namespace sdl3cpp::services::impl {

/**
 * @brief Builds a wheel primitive's two rims as a drawable disc pair.
 *
 * A Wheel's six vertex indices are two groups of three -- one per
 * face -- each a disc centre, a point `radius` above it, and a second
 * point `radius` further round the rim. Every wheel in a retail
 * install has both points offset purely along the shape's Y and Z
 * axes (never X), which is confirmed here rather than assumed: a
 * wheel whose rim points do not lie in that plane is skipped, so a
 * wrong read never turns into a wrong-shaped disc.
 *
 * @param segments How many chords each rim is drawn with.
 */
void AppendStuntsWheelMesh(GeometryPlaneMesh& mesh, const StuntsShape& shape,
                          const StuntsShapeFace& wheel, int segments);

}  // namespace sdl3cpp::services::impl
