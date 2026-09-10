#include "services/interfaces/workflow/rendering/bsp_patch_tessellator.hpp"
#include "services/interfaces/workflow/rendering/bsp_patch_bezier_basis.hpp"
#include "services/interfaces/workflow/rendering/bsp_patch_lightmap_uv.hpp"
#include "services/interfaces/workflow/rendering/bsp_patch_sample.hpp"

namespace sdl3cpp::services::impl {

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
