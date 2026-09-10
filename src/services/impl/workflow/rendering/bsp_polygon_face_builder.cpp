#include "services/interfaces/workflow/rendering/bsp_polygon_face_builder.hpp"

#include <cstdint>

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

void AppendFaceVertices(const BspGeometryLumps& lumps, const BspFace& face,
                        float scale, const LightmapUv& lmUv,
                        TextureGroup& group) {
    for (int vi = 0; vi < face.n_vertices; ++vi) {
        const int srcIdx = face.vertex + vi;
        if (srcIdx < 0 || srcIdx >= lumps.numVertices) {
            continue;
        }
        const auto& sv = lumps.vertices[srcIdx];
        BspRenderVertex rv;
        rv.x    = sv.position[0] * scale;
        rv.y    = sv.position[2] * scale;
        rv.z    = -sv.position[1] * scale;
        rv.u    = sv.texcoord[0][0];
        rv.v    = sv.texcoord[0][1];
        rv.lm_u = lmUv.offsetU + sv.texcoord[1][0] * lmUv.scaleU;
        rv.lm_v = lmUv.offsetV + sv.texcoord[1][1] * lmUv.scaleV;
        rv.nx   = sv.normal[0];
        rv.ny   = sv.normal[2];
        rv.nz   = -sv.normal[1];
        group.vertices.push_back(rv);
    }
}

/// Swaps v1/v2 per triangle to fix Q3's clockwise winding.
void AppendFaceIndices(const BspGeometryLumps& lumps, const BspFace& face,
                       uint32_t baseVertex, TextureGroup& group) {
    for (int mv = 0; mv + 2 < face.n_meshverts; mv += 3) {
        const int i0 = lumps.meshVerts[face.meshvert + mv];
        const int i1 = lumps.meshVerts[face.meshvert + mv + 1];
        const int i2 = lumps.meshVerts[face.meshvert + mv + 2];
        if (i0 < 0 || i0 >= face.n_vertices) continue;
        if (i1 < 0 || i1 >= face.n_vertices) continue;
        if (i2 < 0 || i2 >= face.n_vertices) continue;
        group.indices.push_back(baseVertex + static_cast<uint32_t>(i0));
        group.indices.push_back(baseVertex + static_cast<uint32_t>(i2));
        group.indices.push_back(baseVertex + static_cast<uint32_t>(i1));
    }
}

}  // namespace

void AppendBspPolygonFace(const BspGeometryLumps& lumps, const BspFace& face,
                          float scale, int gridSize, int numLightmaps,
                          TextureGroup& group) {
    const uint32_t baseVertex = static_cast<uint32_t>(group.vertices.size());
    const LightmapUv lmUv =
        ComputeLightmapUv(gridSize, numLightmaps, face.lm_index);
    AppendFaceVertices(lumps, face, scale, lmUv, group);
    AppendFaceIndices(lumps, face, baseVertex, group);
}

}  // namespace sdl3cpp::services::impl
