#pragma once

/// Internals of TraceBoxThroughBrushes(), split out to keep the clip
/// loop and the sweep bookkeeping in separate files.

#include "services/interfaces/workflow/quake3/q3_brush_collision.hpp"

namespace sdl3cpp::services::impl::brush_trace_detail {

/// Pushes a side's plane out by the box's support, so the sweep can be
/// treated as a point. The corner that matters is the one furthest
/// *into* the brush: mins where the normal is positive, maxs where it is
/// negative (ioq3 indexes tw->offsets by the normal's sign bits).
inline float ExpandedDist(const BrushPlane& side, const glm::vec3& mins,
                          const glm::vec3& maxs) {
    float offset = 0.f;
    for (int axis = 0; axis < 3; ++axis) {
        offset += side.normal[axis] *
                  (side.normal[axis] > 0.f ? mins[axis] : maxs[axis]);
    }
    return side.dist - offset;
}

/// Running result across the brushes a sweep is tested against.
struct TraceState {
    float fraction = 1.f;
    glm::vec3 normal{0.f, 1.f, 0.f};
    /// The sweep began inside a brush. Quake reports this so callers can
    /// tell "blocked immediately" from "nothing in the way".
    bool startSolid = false;
    /// It began inside and never got out, so no fraction is meaningful.
    bool allSolid = false;
};

/**
 * @brief Clips one sweep against one brush, updating `state`.
 *
 * ioq3 cm_trace.c CM_TraceThroughBrush(): walk the sides tracking the
 * last plane entered and the first left. If the sweep enters before it
 * leaves, it is inside the brush over that span and the entry plane is
 * what stopped it.
 */
void ClipToBrush(const BrushCollisionModel& model, const CollisionBrush& brush,
                 const glm::vec3& from, const glm::vec3& to,
                 const glm::vec3& mins, const glm::vec3& maxs,
                 TraceState& state);

}  // namespace sdl3cpp::services::impl::brush_trace_detail
