#pragma once

#include "services/interfaces/workflow/quake3/q3_brush_collision.hpp"
#include "services/interfaces/workflow/quake3/q3_trace_result.hpp"

namespace sdl3cpp::services::impl {

/**
 * @brief Sweeps a box through the brush model, Quake's way.
 *
 * Ported from ioq3 cm_trace.c CM_TraceThroughBrush(): each side's plane
 * is pushed out by the box's support along its normal, and the sweep is
 * clipped against the resulting halfspaces, tracking the plane that
 * produced the latest entry. That plane is the surface that stopped the
 * move — known exactly, rather than guessed from a contact point, which
 * is the whole reason for tracing brushes instead of convex hulls.
 *
 * Expanding the side planes is only exact because every q3map2 brush
 * carries its six axial planes; without those bevels a box could clip
 * the brush's edges.
 *
 * @param includePlayerClip Quake's MASK_PLAYERSOLID: pmove is stopped by
 *                          player-clip brushes, shots are not.
 */
Q3Trace TraceBoxThroughBrushes(const BrushCollisionModel& model,
                               glm::vec3 from, glm::vec3 to, glm::vec3 mins,
                               glm::vec3 maxs, bool includePlayerClip = true);

}  // namespace sdl3cpp::services::impl
