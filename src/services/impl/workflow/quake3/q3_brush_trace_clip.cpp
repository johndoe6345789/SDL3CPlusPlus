#include "services/interfaces/workflow/quake3/q3_brush_trace_internal.hpp"

namespace sdl3cpp::services::impl::brush_trace_detail {
namespace {

constexpr float kSurfaceClipEpsilon = 0.125f / 32.0f;

}  // namespace

void ClipToBrush(const BrushCollisionModel& model, const CollisionBrush& brush,
                 const glm::vec3& from, const glm::vec3& to,
                 const glm::vec3& mins, const glm::vec3& maxs,
                 TraceState& state) {
    float enterFrac = -1.f;
    float leaveFrac = 1.f;
    glm::vec3 clipNormal(0.f, 1.f, 0.f);
    bool getOut   = false;
    bool startOut = false;

    for (int s = 0; s < brush.numSides; ++s) {
        const BrushPlane& side = model.sides[brush.firstSide + s];
        const float dist       = ExpandedDist(side, mins, maxs);
        const float d1         = glm::dot(from, side.normal) - dist;
        const float d2         = glm::dot(to, side.normal) - dist;

        // ioq3 tests these against 0. Allowing an epsilon of slack
        // instead means a box resting exactly on a surface counts as
        // outside it rather than inside: Quake never lands exactly on a
        // plane because its traces stop short of one, but a spawn point
        // or a hand-placed origin can, and being judged solid there is
        // the "frozen in the floor" failure this trace exists to end.
        if (d2 > -kSurfaceClipEpsilon) getOut = true;
        if (d1 > -kSurfaceClipEpsilon) startOut = true;

        // Entirely in front of this side, so it misses the whole brush.
        if (d1 > 0.f && (d2 >= kSurfaceClipEpsilon || d2 >= d1)) {
            return;
        }
        // Same epsilon of slack: a box resting exactly on this plane and
        // moving into it is touching it, and must be reported, or a
        // ground probe from a standing position finds nothing underfoot.
        if (d1 <= -kSurfaceClipEpsilon && d2 <= -kSurfaceClipEpsilon) {
            continue;  // never comes near this side
        }

        if (d1 > d2) {  // entering the brush through this side
            const float f = std::max((d1 - kSurfaceClipEpsilon) / (d1 - d2),
                                     0.f);
            if (f > enterFrac) {
                enterFrac  = f;
                clipNormal = side.normal;
            }
        } else {  // leaving through it
            const float f = std::min((d1 + kSurfaceClipEpsilon) / (d1 - d2),
                                     1.f);
            leaveFrac = std::min(leaveFrac, f);
        }
    }

    if (!startOut) {
        state.startSolid = true;
        if (!getOut) state.allSolid = true;
        return;
    }
    if (enterFrac < leaveFrac && enterFrac > -1.f &&
        enterFrac < state.fraction) {
        state.fraction = std::max(enterFrac, 0.f);
        state.normal   = clipNormal;
    }
}

}  // namespace sdl3cpp::services::impl::brush_trace_detail
