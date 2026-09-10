#pragma once

#include "services/interfaces/workflow/rendering/bsp_brush_lump_view.hpp"

namespace sdl3cpp::services::impl {

/// How a brush should be handled when building collision shapes.
enum class BrushKind { Skip, Solid, PlayerClip };

/**
 * @brief Classifies a brush from its shader's contents/flags.
 *
 * Skips brushes whose shader has neither CONTENTS_SOLID nor
 * CONTENTS_PLAYERCLIP, or is SURF_NODRAW (except player-clip, which is
 * always nodraw).
 */
BrushKind ClassifyBrush(const BspBrushLumpView& view, const BspBrush& brush);

}  // namespace sdl3cpp::services::impl
