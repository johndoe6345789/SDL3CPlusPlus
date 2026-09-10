#include "services/interfaces/workflow/quake3/q3_brush_collision.hpp"

#include "services/interfaces/workflow/rendering/bsp_brush_classify.hpp"
#include "services/interfaces/workflow/rendering/bsp_brush_lump_view.hpp"

#include <cmath>
#include <limits>

namespace sdl3cpp::services::impl {
namespace {

/// bsp.load maps Quake's Z-up space to the engine's Y-up one as
/// (x, y, z) -> (x, z, -y), scaled. That is a rotation, so a plane's
/// normal takes the same map and its distance only scales.
BrushPlane ToEnginePlane(const BspPlane& plane, float scale) {
    BrushPlane out;
    out.normal = glm::vec3(plane.normal[0], plane.normal[2], -plane.normal[1]);
    out.dist   = plane.dist * scale;
    return out;
}

/// Reads the brush's extent off its axial sides. Every q3map2 brush
/// carries all six, which is also what makes expanding the planes by a
/// box exact rather than approximate.
void AccumulateBounds(const BrushPlane& side, glm::vec3& mins,
                      glm::vec3& maxs) {
    constexpr float kAxial = 0.999f;
    for (int axis = 0; axis < 3; ++axis) {
        if (side.normal[axis] > kAxial) {
            maxs[axis] = side.dist;
        } else if (side.normal[axis] < -kAxial) {
            mins[axis] = -side.dist;
        }
    }
}

}  // namespace

BrushCollisionModel BuildBrushCollisionModel(
    const std::vector<uint8_t>& bspData, float scale) {
    BrushCollisionModel model;
    const BspBrushLumpView view = ParseBspBrushLumps(bspData);

    for (int b = 0; b < view.numBrushes; ++b) {
        const BspBrush& brush = view.brushes[b];
        const BrushKind kind  = ClassifyBrush(view, brush);
        const bool boundsOk =
            brush.firstSide >= 0 &&
            brush.firstSide + brush.numSides <= view.numBrushSides;
        if (kind == BrushKind::Skip || !boundsOk || brush.numSides <= 0) {
            continue;
        }

        CollisionBrush out;
        out.firstSide  = static_cast<int>(model.sides.size());
        out.numSides   = brush.numSides;
        out.playerClip = kind == BrushKind::PlayerClip;
        out.mins = glm::vec3(std::numeric_limits<float>::max());
        out.maxs = glm::vec3(std::numeric_limits<float>::lowest());

        for (int s = 0; s < brush.numSides; ++s) {
            const BspBrushSide& side = view.brushSides[brush.firstSide + s];
            const BrushPlane plane =
                ToEnginePlane(view.planes[side.planeIndex], scale);
            model.sides.push_back(plane);
            AccumulateBounds(plane, out.mins, out.maxs);
        }

        if (out.mins.x > out.maxs.x) {
            model.sides.resize(static_cast<size_t>(out.firstSide));
            continue;  // no axial sides: not a brush we can bound
        }
        model.brushes.push_back(out);
    }
    return model;
}

}  // namespace sdl3cpp::services::impl
