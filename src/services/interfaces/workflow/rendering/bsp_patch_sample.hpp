#pragma once

#include "services/interfaces/workflow/rendering/bsp_patch_bezier_basis.hpp"
#include "services/interfaces/workflow/rendering/bsp_types.hpp"

namespace sdl3cpp::services::impl {

/// Samples the patch surface (and its two tangents) at parameter (s, t).
void SamplePatch(const BspVertex controlPoints[9], BezierWeights bs,
                 BezierWeights bt, BezierWeights dbs, BezierWeights dbt,
                 float outPos[3], float outUv[2], float outLmUv[2],
                 float outTangentS[3], float outTangentT[3]);

/// Derives a unit surface normal from the cross product of the two
/// tangents; left as zero if the tangents are near-degenerate.
void NormalFromTangents(const float tangentS[3], const float tangentT[3],
                        float outNormal[3]);

}  // namespace sdl3cpp::services::impl
