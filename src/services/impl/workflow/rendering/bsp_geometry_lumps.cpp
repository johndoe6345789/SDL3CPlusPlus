#include "services/interfaces/workflow/rendering/bsp_geometry_lumps.hpp"

namespace sdl3cpp::services::impl {

BspGeometryLumps ReadBspGeometryLumps(const std::vector<uint8_t>& bspData) {
    const auto* lumps =
        reinterpret_cast<const BspLump*>(bspData.data() + sizeof(BspHeader));

    BspGeometryLumps view;

    const auto& vertexLump = lumps[LUMP_VERTICES];
    view.numVertices = vertexLump.length / static_cast<int>(sizeof(BspVertex));
    view.vertices =
        reinterpret_cast<const BspVertex*>(bspData.data() + vertexLump.offset);

    const auto& faceLump = lumps[LUMP_FACES];
    view.numFaces        = faceLump.length / static_cast<int>(sizeof(BspFace));
    view.faces =
        reinterpret_cast<const BspFace*>(bspData.data() + faceLump.offset);

    const auto& meshVertLump = lumps[LUMP_MESHVERTS];
    view.meshVerts =
        reinterpret_cast<const int32_t*>(bspData.data() + meshVertLump.offset);

    const auto& textureLump = lumps[LUMP_TEXTURES];
    view.numTextures =
        textureLump.length / static_cast<int>(sizeof(BspTexture));
    view.textures = reinterpret_cast<const BspTexture*>(bspData.data() +
                                                        textureLump.offset);

    return view;
}

}  // namespace sdl3cpp::services::impl
