#pragma once

#include <btBulletDynamicsCommon.h>

#include <vector>

namespace sdl3cpp::services::impl {

/// Builds a margined, polyhedral-featured convex hull shape from a brush's
/// intersection-derived vertices. Caller owns the returned shape.
btConvexHullShape* BuildBrushConvexShape(
    const std::vector<btVector3>& hullVerts);

}  // namespace sdl3cpp::services::impl
