#pragma once

#include "services/interfaces/workflow/quake3/q3_mapselect_assets.hpp"

namespace sdl3cpp::services::impl {

/// Draws the left/right arrow icons when there is a previous/next map.
void DrawNavArrows(const Q3MapSelectAssets& a, int idx, int nMaps, float y);

/// Draws the back/skirmish/fight button row along the bottom edge.
void DrawMapSelectButtons(const Q3MapSelectAssets& a);

}  // namespace sdl3cpp::services::impl
