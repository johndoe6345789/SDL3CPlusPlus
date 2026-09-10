#pragma once

#include "services/interfaces/workflow/quake3/q3_mapselect_assets.hpp"

#include <string>

namespace sdl3cpp::services::impl {

/// Draws the arena long-name (and, if the arena has a bot, its icon and
/// upper-cased name) centered under the levelshot. No-op if `a.arenas`
/// has no entry for `mapName` or the entry has no long name.
void DrawArenaInfo(const Q3MapSelectAssets& a, const std::string& mapName,
                   float centerX, float infoY);

}  // namespace sdl3cpp::services::impl
