#include "services/interfaces/workflow/rendering/bsp_patch_tessellator.hpp"

#include <cmath>

namespace sdl3cpp::services::impl {
namespace {

struct LightmapUv {
    float offsetU = 0.0f, offsetV = 0.0f;
    float scaleU = 1.0f, scaleV = 1.0f;
};

LightmapUv ComputeLightmapUv(int gridSize, int numLightmaps, int lmIndex) {
    LightmapUv uv;
    uv.scaleU = 1.0f / static_cast<float>(gridSize);
    uv.scaleV = 1.0f / static_cast<float>(gridSize);
    if (lmIndex >= 0 && lmIndex < numLightmaps) {
        const int slot  = lmIndex + 1;
        const int slotX = slot % gridSize;
        const int slotY = slot / gridSize;
        uv.offsetU = static_cast<float>(slotX) / static_cast<float>(gridSize);
        uv.offsetV = static_cast<float>(slotY) / static_cast<float>(gridSize);
    }
    return uv;
}

/// Quadratic Bezier basis and derivative weights for a single parameter.
struct BezierWeights {
    float w0, w1, w2;
};

BezierWeights BasisWeights(float t) {
    return {(1.0f - t) * (1.0f - t), 2.0f * (1.0f - t) * t, t * t};
}

BezierWeights DerivativeWeights(float t) {
    return {-2.0f * (1.0f - t), 2.0f - 4.0f * t, 2.0f * t};
}

/// Samples the patch surface (and its two tangents) at parameter (s, t).
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

}  // namespace

void TessellateBspPatch(const BspVertex controlPoints[9], int level,
                        float scale, int gridSize, int numLightmaps,
                        int lmIndex, std::vector<BspRenderVertex>& outVertices,
                        std::vector<uint32_t>& outIndices) {
    const int steps       = level + 1;
    const LightmapUv lmUv = ComputeLightmapUv(gridSize, numLightmaps, lmIndex);
    const uint32_t baseVertex = static_cast<uint32_t>(outVertices.size());

    for (int row = 0; row < steps; ++row) {
        const float t  = static_cast<float>(row) / static_cast<float>(level);
        const auto bt  = BasisWeights(t);
        const auto dbt = DerivativeWeights(t);

        for (int col = 0; col < steps; ++col) {
            const float s = static_cast<float>(col) / static_cast<float>(level);
            const auto bs = BasisWeights(s);
            const auto dbs = DerivativeWeights(s);

            float pos[3], uv[2], lmuv[2], tangentS[3], tangentT[3], normal[3];
            SamplePatch(controlPoints, bs, bt, dbs, dbt, pos, uv, lmuv,
                        tangentS, tangentT);
            NormalFromTangents(tangentS, tangentT, normal);

            BspRenderVertex rv;
            rv.x    = pos[0] * scale;
            rv.y    = pos[2] * scale;
            rv.z    = -pos[1] * scale;
            rv.u    = uv[0];
            rv.v    = uv[1];
            rv.lm_u = lmUv.offsetU + lmuv[0] * lmUv.scaleU;
            rv.lm_v = lmUv.offsetV + lmuv[1] * lmUv.scaleV;
            rv.nx   = normal[0];
            rv.ny   = normal[2];
            rv.nz   = -normal[1];
            outVertices.push_back(rv);
        }
    }

    for (int row = 0; row < level; ++row) {
        for (int col = 0; col < level; ++col) {
            const uint32_t tl =
                baseVertex + static_cast<uint32_t>(row * steps + col);
            const uint32_t tr = tl + 1;
            const uint32_t bl = tl + static_cast<uint32_t>(steps);
            const uint32_t br = bl + 1;

            outIndices.push_back(tl);
            outIndices.push_back(tr);
            outIndices.push_back(bl);

            outIndices.push_back(tr);
            outIndices.push_back(br);
            outIndices.push_back(bl);
        }
    }
}

}  // namespace sdl3cpp::services::impl
