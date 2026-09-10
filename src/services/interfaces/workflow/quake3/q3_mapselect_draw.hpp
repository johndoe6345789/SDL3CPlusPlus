#pragma once

#include "services/interfaces/workflow/quake3/q3_mapselect_assets.hpp"

namespace sdl3cpp::services::impl {

/// Renders the full "CHOOSE LEVEL" screen: title, levelshot (loaded from
/// the pk3 and cached in `assets.shots` on first use), map/arena name,
/// bot icon, nav arrows, and the back/skirmish/fight buttons.
void DrawQ3MapSelectScreen(const Q3MapSelectAssets& assets);

}  // namespace sdl3cpp::services::impl
