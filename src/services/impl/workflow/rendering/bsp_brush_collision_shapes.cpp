#include "services/interfaces/workflow/rendering/bsp_brush_collision_shapes.hpp"
#include "services/interfaces/workflow/rendering/bsp_brush_classify.hpp"
#include "services/interfaces/workflow/rendering/bsp_brush_convex_shape.hpp"
#include "services/interfaces/workflow/rendering/bsp_brush_lump_view.hpp"
#include "services/interfaces/workflow/rendering/bsp_brush_vertex_hull.hpp"

namespace sdl3cpp::services::impl {

BspBrushCollisionShapes BuildBspBrushCollisionShapes(
    const std::vector<uint8_t>& bspData, float scale) {
    BspBrushCollisionShapes out;
    const BspBrushLumpView view = ParseBspBrushLumps(bspData);

    out.solid = new btCompoundShape();
    // Player-clip brushes go in their own body so that only pmove sees
    // them, matching Quake's MASK_PLAYERSOLID / MASK_SHOT split.
    auto* clipCompound = new btCompoundShape();

    for (int b = 0; b < view.numBrushes; ++b) {
        const auto& brush    = view.brushes[b];
        const BrushKind kind = ClassifyBrush(view, brush);
        const bool boundsOk  = brush.firstSide >= 0 &&
                               brush.firstSide + brush.numSides <=
                                   view.numBrushSides;
        if (kind == BrushKind::Skip || !boundsOk) {
            ++out.skippedBrushes;
            continue;
        }

        auto hullVerts = ComputeBrushVertices(&view.brushSides[brush.firstSide],
                                              brush.numSides, view.planes,
                                              scale);
        if (hullVerts.size() < 4) {
            ++out.skippedBrushes;
            continue;
        }

        auto* convex = BuildBrushConvexShape(hullVerts);

        btTransform childTransform;
        childTransform.setIdentity();
        if (kind == BrushKind::PlayerClip) {
            clipCompound->addChildShape(childTransform, convex);
            ++out.clipBrushes;
        } else {
            out.solid->addChildShape(childTransform, convex);
            ++out.solidBrushes;
        }
    }

    if (out.clipBrushes > 0) {
        out.clip = clipCompound;
    } else {
        delete clipCompound;
    }

    return out;
}

}  // namespace sdl3cpp::services::impl
