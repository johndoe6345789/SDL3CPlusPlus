#include "services/interfaces/workflow/rendering/bsp_patch_sample.hpp"

#include <cmath>

namespace sdl3cpp::services::impl {

void SamplePatch(const BspVertex controlPoints[9], BezierWeights bs,
                 BezierWeights bt, BezierWeights dbs, BezierWeights dbt,
                 float outPos[3], float outUv[2], float outLmUv[2],
                 float outTangentS[3], float outTangentT[3]) {
    const float weight[9]   = {bt.w0 * bs.w0, bt.w0 * bs.w1, bt.w0 * bs.w2,
                               bt.w1 * bs.w0, bt.w1 * bs.w1, bt.w1 * bs.w2,
                               bt.w2 * bs.w0, bt.w2 * bs.w1, bt.w2 * bs.w2};
    const float dsWeight[9] = {bt.w0 * dbs.w0, bt.w0 * dbs.w1, bt.w0 * dbs.w2,
                               bt.w1 * dbs.w0, bt.w1 * dbs.w1, bt.w1 * dbs.w2,
                               bt.w2 * dbs.w0, bt.w2 * dbs.w1, bt.w2 * dbs.w2};
    const float dtWeight[9] = {dbt.w0 * bs.w0, dbt.w0 * bs.w1, dbt.w0 * bs.w2,
                               dbt.w1 * bs.w0, dbt.w1 * bs.w1, dbt.w1 * bs.w2,
                               dbt.w2 * bs.w0, dbt.w2 * bs.w1, dbt.w2 * bs.w2};

    for (int i = 0; i < 3; ++i) {
        outPos[i]      = 0.0f;
        outTangentS[i] = 0.0f;
        outTangentT[i] = 0.0f;
    }
    outUv[0] = outUv[1] = outLmUv[0] = outLmUv[1] = 0.0f;

    for (int i = 0; i < 9; ++i) {
        const auto& cp = controlPoints[i];
        for (int axis = 0; axis < 3; ++axis) {
            outPos[axis] += cp.position[axis] * weight[i];
            outTangentS[axis] += cp.position[axis] * dsWeight[i];
            outTangentT[axis] += cp.position[axis] * dtWeight[i];
        }
        outUv[0] += cp.texcoord[0][0] * weight[i];
        outUv[1] += cp.texcoord[0][1] * weight[i];
        outLmUv[0] += cp.texcoord[1][0] * weight[i];
        outLmUv[1] += cp.texcoord[1][1] * weight[i];
    }
}

void NormalFromTangents(const float tangentS[3], const float tangentT[3],
                        float outNormal[3]) {
    outNormal[0] = tangentS[1] * tangentT[2] - tangentS[2] * tangentT[1];
    outNormal[1] = tangentS[2] * tangentT[0] - tangentS[0] * tangentT[2];
    outNormal[2] = tangentS[0] * tangentT[1] - tangentS[1] * tangentT[0];
    const float len =
        std::sqrt(outNormal[0] * outNormal[0] + outNormal[1] * outNormal[1] +
                  outNormal[2] * outNormal[2]);
    if (len > 1e-6f) {
        outNormal[0] /= len;
        outNormal[1] /= len;
        outNormal[2] /= len;
    }
}

}  // namespace sdl3cpp::services::impl
