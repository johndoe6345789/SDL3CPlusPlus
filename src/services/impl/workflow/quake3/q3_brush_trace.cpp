#include "services/interfaces/workflow/quake3/q3_brush_trace.hpp"

#include "services/interfaces/workflow/quake3/q3_brush_trace_internal.hpp"

#include <algorithm>
#include <cmath>

namespace sdl3cpp::services::impl {
namespace {

/// cm_local.h SURFACE_CLIP_EPSILON, in engine units. A trace stops just
/// short of the surface so the next one never starts in contact.
constexpr float kSurfaceClipEpsilon = 0.125f / 32.0f;

bool BoundsMiss(const CollisionBrush& brush, const glm::vec3& sweepMins,
                const glm::vec3& sweepMaxs) {
    for (int axis = 0; axis < 3; ++axis) {
        if (brush.mins[axis] > sweepMaxs[axis] ||
            brush.maxs[axis] < sweepMins[axis]) {
            return true;
        }
    }
    return false;
}

}  // namespace

Q3Trace TraceBoxThroughBrushes(const BrushCollisionModel& model,
                               glm::vec3 from, glm::vec3 to, glm::vec3 mins,
                               glm::vec3 maxs, bool includePlayerClip) {
    Q3Trace trace;
    trace.endPos = to;

    // The swept box's bounds, for rejecting brushes without any plane
    // work at all.
    glm::vec3 sweepMins = glm::min(from, to) + mins;
    glm::vec3 sweepMaxs = glm::max(from, to) + maxs;

    brush_trace_detail::TraceState state;
    for (const CollisionBrush& brush : model.brushes) {
        if (brush.playerClip && !includePlayerClip) {
            continue;
        }
        if (BoundsMiss(brush, sweepMins, sweepMaxs)) {
            continue;
        }
        brush_trace_detail::ClipToBrush(model, brush, from, to, mins, maxs,
                                        state);
        if (state.allSolid) {
            break;
        }
    }

    trace.startSolid = state.startSolid;
    if (state.allSolid) {
        trace.hit      = true;
        trace.fraction = 0.f;
        trace.endPos   = from;
        return trace;
    }
    if (state.fraction >= 1.f) {
        return trace;  // reached the target
    }

    trace.hit      = true;
    trace.fraction = state.fraction;
    trace.normal   = state.normal;
    // Stop short of the surface, as Quake's trace does, so the next
    // sweep does not begin in contact with it.
    trace.endPos =
        from + (to - from) * state.fraction + state.normal * kSurfaceClipEpsilon;
    return trace;
}

}  // namespace sdl3cpp::services::impl
