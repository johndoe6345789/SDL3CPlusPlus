#include "services/interfaces/workflow/rendering/bsp_brush_classify.hpp"

namespace sdl3cpp::services::impl {

BrushKind ClassifyBrush(const BspBrushLumpView& view, const BspBrush& brush) {
    if (brush.shaderIndex < 0 || brush.shaderIndex >= view.numTextures) {
        return BrushKind::Skip;
    }
    const auto& tex       = view.textures[brush.shaderIndex];
    const bool playerClip = (tex.contents & CONTENTS_PLAYERCLIP) != 0;
    if (!(tex.contents & CONTENTS_SOLID) && !playerClip) {
        return BrushKind::Skip;
    }
    // SURF_NODRAW is a rendering property. Player-clip brushes are always
    // nodraw, so skipping on it dropped exactly the geometry that makes
    // curved surfaces walkable.
    if ((tex.flags & SURF_NODRAW) && !playerClip) {
        return BrushKind::Skip;
    }
    return playerClip ? BrushKind::PlayerClip : BrushKind::Solid;
}

}  // namespace sdl3cpp::services::impl
