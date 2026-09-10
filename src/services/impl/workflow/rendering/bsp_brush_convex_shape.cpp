#include "services/interfaces/workflow/rendering/bsp_brush_convex_shape.hpp"

namespace sdl3cpp::services::impl {

btConvexHullShape* BuildBrushConvexShape(
    const std::vector<btVector3>& hullVerts) {
    auto* convex = new btConvexHullShape();
    for (const auto& v : hullVerts) {
        convex->addPoint(v, false);
    }
    convex->recalcLocalAabb();
    convex->setMargin(0.01f);
    // Keep the brush's own face planes so pmove traces can report a face
    // normal rather than Bullet's edge separation direction.
    convex->initializePolyhedralFeatures();
    return convex;
}

}  // namespace sdl3cpp::services::impl
