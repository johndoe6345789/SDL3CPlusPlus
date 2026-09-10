#pragma once

#include "services/interfaces/workflow/rendering/bsp_types.hpp"

#include <btBulletDynamicsCommon.h>

#include <vector>

namespace sdl3cpp::services::impl {

/**
 * @brief Computes convex hull vertices from plane intersections of a brush.
 *
 * Intersects every triple of the brush's face planes and keeps the points
 * that lie on the inside of every other plane, i.e. the brush's own convex
 * hull expressed as points rather than half-spaces.
 */
std::vector<btVector3> ComputeBrushVertices(const BspBrushSide* sides,
                                            int numSides,
                                            const BspPlane* allPlanes,
                                            float scale);

}  // namespace sdl3cpp::services::impl
