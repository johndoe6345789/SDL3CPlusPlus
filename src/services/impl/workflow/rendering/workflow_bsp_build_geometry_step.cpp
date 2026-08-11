#include "services/interfaces/workflow/rendering/workflow_bsp_build_geometry_step.hpp"
#include "services/interfaces/workflow/rendering/bsp_types.hpp"
#include "services/interfaces/workflow/workflow_step_parameter_resolver.hpp"

#include <nlohmann/json.hpp>
#include <cmath>
#include <memory>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>

namespace sdl3cpp::services::impl {

// Tessellate a single 3x3 biquadratic bezier sub-patch into a grid of triangles.
static void TessellatePatch(
    const BspVertex controlPoints[9],
    int level, float scale,
    int gridSize, int numLightmaps,
    int lmIndex,
    std::vector<BspRenderVertex>& outVertices,
    std::vector<uint32_t>& outIndices)
{
    int steps = level + 1;

    float lmOffsetU = 0.0f, lmOffsetV = 0.0f;
    float lmScaleU = 1.0f / static_cast<float>(gridSize);
    float lmScaleV = 1.0f / static_cast<float>(gridSize);

    if (lmIndex >= 0 && lmIndex < numLightmaps) {
        int slot = lmIndex + 1;
        int slotX = slot % gridSize;
        int slotY = slot / gridSize;
        lmOffsetU = static_cast<float>(slotX) / static_cast<float>(gridSize);
        lmOffsetV = static_cast<float>(slotY) / static_cast<float>(gridSize);
    }

    uint32_t baseVertex = static_cast<uint32_t>(outVertices.size());

    for (int row = 0; row < steps; ++row) {
        float t = static_cast<float>(row) / static_cast<float>(level);
        float bt0 = (1.0f - t) * (1.0f - t);
        float bt1 = 2.0f * (1.0f - t) * t;
        float bt2 = t * t;

        for (int col = 0; col < steps; ++col) {
            float s = static_cast<float>(col) / static_cast<float>(level);
            float bs0 = (1.0f - s) * (1.0f - s);
            float bs1 = 2.0f * (1.0f - s) * s;
            float bs2 = s * s;

            float pos[3] = {0, 0, 0};
            float uv[2] = {0, 0};
            float lmuv[2] = {0, 0};

            float weights[9] = {
                bt0 * bs0, bt0 * bs1, bt0 * bs2,
                bt1 * bs0, bt1 * bs1, bt1 * bs2,
                bt2 * bs0, bt2 * bs1, bt2 * bs2
            };

            for (int i = 0; i < 9; ++i) {
                float w = weights[i];
                pos[0] += controlPoints[i].position[0] * w;
                pos[1] += controlPoints[i].position[1] * w;
                pos[2] += controlPoints[i].position[2] * w;
                uv[0]  += controlPoints[i].texcoord[0][0] * w;
                uv[1]  += controlPoints[i].texcoord[0][1] * w;
                lmuv[0] += controlPoints[i].texcoord[1][0] * w;
                lmuv[1] += controlPoints[i].texcoord[1][1] * w;
            }

            float dbs0 = -2.0f * (1.0f - s);
            float dbs1 = 2.0f - 4.0f * s;
            float dbs2 = 2.0f * s;

            float dbt0 = -2.0f * (1.0f - t);
            float dbt1 = 2.0f - 4.0f * t;
            float dbt2 = 2.0f * t;

            float tangentS[3] = {0, 0, 0};
            float tangentT[3] = {0, 0, 0};

            float dsWeights[9] = {
                bt0 * dbs0, bt0 * dbs1, bt0 * dbs2,
                bt1 * dbs0, bt1 * dbs1, bt1 * dbs2,
                bt2 * dbs0, bt2 * dbs1, bt2 * dbs2
            };
            float dtWeights[9] = {
                dbt0 * bs0, dbt0 * bs1, dbt0 * bs2,
                dbt1 * bs0, dbt1 * bs1, dbt1 * bs2,
                dbt2 * bs0, dbt2 * bs1, dbt2 * bs2
            };

            for (int i = 0; i < 9; ++i) {
                tangentS[0] += controlPoints[i].position[0] * dsWeights[i];
                tangentS[1] += controlPoints[i].position[1] * dsWeights[i];
                tangentS[2] += controlPoints[i].position[2] * dsWeights[i];
                tangentT[0] += controlPoints[i].position[0] * dtWeights[i];
                tangentT[1] += controlPoints[i].position[1] * dtWeights[i];
                tangentT[2] += controlPoints[i].position[2] * dtWeights[i];
            }

            float nx = tangentS[1] * tangentT[2] - tangentS[2] * tangentT[1];
            float ny = tangentS[2] * tangentT[0] - tangentS[0] * tangentT[2];
            float nz = tangentS[0] * tangentT[1] - tangentS[1] * tangentT[0];
            float len = std::sqrt(nx * nx + ny * ny + nz * nz);
            if (len > 1e-6f) { nx /= len; ny /= len; nz /= len; }

            BspRenderVertex rv;
            rv.x = pos[0] * scale;
            rv.y = pos[2] * scale;
            rv.z = -pos[1] * scale;

            rv.u = uv[0];
            rv.v = uv[1];

            rv.lm_u = lmOffsetU + lmuv[0] * lmScaleU;
            rv.lm_v = lmOffsetV + lmuv[1] * lmScaleV;

            rv.nx = nx;
            rv.ny = nz;
            rv.nz = -ny;

            outVertices.push_back(rv);
        }
    }

    for (int row = 0; row < level; ++row) {
        for (int col = 0; col < level; ++col) {
            uint32_t tl = baseVertex + static_cast<uint32_t>(row * steps + col);
            uint32_t tr = tl + 1;
            uint32_t bl = tl + static_cast<uint32_t>(steps);
            uint32_t br = bl + 1;

            outIndices.push_back(tl);
            outIndices.push_back(tr);
            outIndices.push_back(bl);

            outIndices.push_back(tr);
            outIndices.push_back(br);
            outIndices.push_back(bl);
        }
    }
}

WorkflowBspBuildGeometryStep::WorkflowBspBuildGeometryStep(std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)) {}

std::string WorkflowBspBuildGeometryStep::GetPluginId() const {
    return "bsp.build_geometry";
}

void WorkflowBspBuildGeometryStep::Execute(const WorkflowStepDefinition& step, WorkflowContext& context) {
    WorkflowStepParameterResolver params;

    auto bspDataPtr = context.Get<std::shared_ptr<std::vector<uint8_t>>>("bsp_raw_data", nullptr);
    if (!bspDataPtr) throw std::runtime_error("bsp.build_geometry: bsp_raw_data not in context");

    auto bspConfig = context.Get<nlohmann::json>("bsp_config", nlohmann::json{});
    float scale = bspConfig.value("scale", 1.0f / 32.0f);
    std::string map_name = bspConfig.value("map_name", std::string("q3dm17"));

    int gridSize = context.Get<int>("bsp_grid_size", 1);
    int numLightmaps = context.Get<int>("bsp_num_lightmaps", 0);

    const auto* p = params.FindParameter(step, "patch_tess_level");
    int patchTessLevel = (p && p->type == WorkflowParameterValue::Type::Number)
        ? static_cast<int>(p->numberValue) : 4;

    const auto& bspData = *bspDataPtr;
    auto* lumps = reinterpret_cast<const BspLump*>(bspData.data() + sizeof(BspHeader));

    const auto& vtxLump = lumps[LUMP_VERTICES];
    int numVertices = vtxLump.length / static_cast<int>(sizeof(BspVertex));
    auto* bspVertices = reinterpret_cast<const BspVertex*>(bspData.data() + vtxLump.offset);

    const auto& faceLump = lumps[LUMP_FACES];
    int numFaces = faceLump.length / static_cast<int>(sizeof(BspFace));
    auto* bspFaces = reinterpret_cast<const BspFace*>(bspData.data() + faceLump.offset);

    const auto& mvLump = lumps[LUMP_MESHVERTS];
    auto* meshVerts = reinterpret_cast<const int32_t*>(bspData.data() + mvLump.offset);

    const auto& texLump = lumps[LUMP_TEXTURES];
    int numTextures = texLump.length / static_cast<int>(sizeof(BspTexture));
    auto* bspTextures = reinterpret_cast<const BspTexture*>(bspData.data() + texLump.offset);

    // Build per-face geometry grouped by texture
    std::map<int, TextureGroup> groups;
    int tessellatedPatches = 0;

    for (int f = 0; f < numFaces; ++f) {
        const auto& face = bspFaces[f];
        if (face.type != 1 && face.type != 2 && face.type != 3) continue;

        bool isVisible = true;
        if (face.texture >= 0 && face.texture < numTextures) {
            const auto& tex = bspTextures[face.texture];
            std::string texName(tex.name);
            if (texName.find("sky") != std::string::npos) isVisible = false;
            if (texName.find("trigger") != std::string::npos) isVisible = false;
            if (texName.find("hint") != std::string::npos) isVisible = false;
            if (texName.find("caulk") != std::string::npos) isVisible = false;
            if (texName.find("clip") != std::string::npos) isVisible = false;
            if (texName.find("nodraw") != std::string::npos) isVisible = false;
            if (texName.find("areaportal") != std::string::npos) isVisible = false;
        }
        if (!isVisible) continue;

        int texIdx = face.texture;
        auto& group = groups[texIdx];

        // Bezier patch (type 2)
        if (face.type == 2) {
            int patchW = face.size[0];
            int patchH = face.size[1];
            if (patchW < 3 || patchH < 3) continue;

            int numSubPatchesX = (patchW - 1) / 2;
            int numSubPatchesY = (patchH - 1) / 2;

            std::vector<BspVertex> grid(patchW * patchH);
            for (int i = 0; i < patchW * patchH && i < face.n_vertices; ++i) {
                int srcIdx = face.vertex + i;
                if (srcIdx >= 0 && srcIdx < numVertices) {
                    grid[i] = bspVertices[srcIdx];
                }
            }

            for (int py = 0; py < numSubPatchesY; ++py) {
                for (int px = 0; px < numSubPatchesX; ++px) {
                    BspVertex cp[9];
                    for (int r = 0; r < 3; ++r) {
                        for (int c = 0; c < 3; ++c) {
                            cp[r * 3 + c] = grid[(py * 2 + r) * patchW + (px * 2 + c)];
                        }
                    }
                    TessellatePatch(cp, patchTessLevel, scale,
                                    gridSize, numLightmaps, face.lm_index,
                                    group.vertices, group.indices);
                }
            }
            ++tessellatedPatches;
            continue;
        }

        // Polygon / mesh face (type 1, 3) — CW→CCW winding fix
        float lmOffsetU = 0.0f, lmOffsetV = 0.0f;
        float lmScaleU = 1.0f / static_cast<float>(gridSize);
        float lmScaleV = 1.0f / static_cast<float>(gridSize);

        if (face.lm_index >= 0 && face.lm_index < numLightmaps) {
            int slot = face.lm_index + 1;
            int slotX = slot % gridSize;
            int slotY = slot / gridSize;
            lmOffsetU = static_cast<float>(slotX) / static_cast<float>(gridSize);
            lmOffsetV = static_cast<float>(slotY) / static_cast<float>(gridSize);
        }

        uint32_t baseVertex = static_cast<uint32_t>(group.vertices.size());

        for (int vi = 0; vi < face.n_vertices; ++vi) {
            int srcIdx = face.vertex + vi;
            if (srcIdx < 0 || srcIdx >= numVertices) continue;

            const auto& sv = bspVertices[srcIdx];
            BspRenderVertex rv;
            rv.x = sv.position[0] * scale;
            rv.y = sv.position[2] * scale;
            rv.z = -sv.position[1] * scale;
            rv.u = sv.texcoord[0][0];
            rv.v = sv.texcoord[0][1];
            rv.lm_u = lmOffsetU + sv.texcoord[1][0] * lmScaleU;
            rv.lm_v = lmOffsetV + sv.texcoord[1][1] * lmScaleV;
            rv.nx = sv.normal[0];
            rv.ny = sv.normal[2];
            rv.nz = -sv.normal[1];
            group.vertices.push_back(rv);
        }

        // Per-triangle with v1↔v2 swap (Q3 CW → our CCW)
        for (int mv = 0; mv + 2 < face.n_meshverts; mv += 3) {
            int i0 = meshVerts[face.meshvert + mv];
            int i1 = meshVerts[face.meshvert + mv + 1];
            int i2 = meshVerts[face.meshvert + mv + 2];
            if (i0 < 0 || i0 >= face.n_vertices) continue;
            if (i1 < 0 || i1 >= face.n_vertices) continue;
            if (i2 < 0 || i2 >= face.n_vertices) continue;
            group.indices.push_back(baseVertex + static_cast<uint32_t>(i0));
            group.indices.push_back(baseVertex + static_cast<uint32_t>(i2));
            group.indices.push_back(baseVertex + static_cast<uint32_t>(i1));
        }
    }

    // Flatten groups into single VB + IB with per-group ranges
    auto allVertices = std::make_shared<std::vector<BspRenderVertex>>();
    auto allIndices = std::make_shared<std::vector<uint32_t>>();
    nlohmann::json mapNodes = nlohmann::json::array();
    auto usedTextures = std::make_shared<std::set<int>>();

    for (auto& [texIdx, group] : groups) {
        if (group.indices.empty()) continue;

        uint32_t vertexOffset = static_cast<uint32_t>(allVertices->size());
        uint32_t indexOffset = static_cast<uint32_t>(allIndices->size());

        allVertices->insert(allVertices->end(), group.vertices.begin(), group.vertices.end());

        for (uint32_t idx : group.indices) {
            allIndices->push_back(idx + vertexOffset);
        }

        usedTextures->insert(texIdx);

        mapNodes.push_back({
            {"name", "bsp_" + map_name},
            {"texture_index", texIdx},
            {"texture_name", (texIdx >= 0 && texIdx < numTextures) ? std::string(bspTextures[texIdx].name) : std::string{}},
            {"index_offset", indexOffset},
            {"index_count", group.indices.size()}
        });
    }

    if (allVertices->empty() || allIndices->empty()) {
        throw std::runtime_error("bsp.build_geometry: No renderable geometry found");
    }

    context.Set("bsp_all_vertices", allVertices);
    context.Set("bsp_all_indices", allIndices);
    context.Set("bsp_used_textures", usedTextures);
    context.Set("map.nodes", mapNodes);

    if (logger_) {
        logger_->Info("bsp.build_geometry: " + std::to_string(allVertices->size()) + " vertices, " +
                     std::to_string(allIndices->size()) + " indices, " +
                     std::to_string(mapNodes.size()) + " texture groups, " +
                     std::to_string(numFaces) + " faces (" +
                     std::to_string(tessellatedPatches) + " patches tessellated)");
    }
}

}  // namespace sdl3cpp::services::impl
