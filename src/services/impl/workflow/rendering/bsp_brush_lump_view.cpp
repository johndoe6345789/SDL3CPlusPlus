#include "services/interfaces/workflow/rendering/bsp_brush_lump_view.hpp"

namespace sdl3cpp::services::impl {

BspBrushLumpView ParseBspBrushLumps(const std::vector<uint8_t>& bspData) {
    auto* lumps =
        reinterpret_cast<const BspLump*>(bspData.data() + sizeof(BspHeader));

    BspBrushLumpView view;

    const auto& texLump = lumps[LUMP_TEXTURES];
    view.numTextures = texLump.length / static_cast<int>(sizeof(BspTexture));
    view.textures =
        reinterpret_cast<const BspTexture*>(bspData.data() + texLump.offset);

    const auto& brushLump = lumps[LUMP_BRUSHES];
    view.numBrushes = brushLump.length / static_cast<int>(sizeof(BspBrush));
    view.brushes =
        reinterpret_cast<const BspBrush*>(bspData.data() + brushLump.offset);

    const auto& brushSideLump = lumps[LUMP_BRUSHSIDES];
    view.numBrushSides =
        brushSideLump.length / static_cast<int>(sizeof(BspBrushSide));
    view.brushSides = reinterpret_cast<const BspBrushSide*>(
        bspData.data() + brushSideLump.offset);

    const auto& planeLump = lumps[LUMP_PLANES];
    view.planes =
        reinterpret_cast<const BspPlane*>(bspData.data() + planeLump.offset);

    return view;
}

}  // namespace sdl3cpp::services::impl
