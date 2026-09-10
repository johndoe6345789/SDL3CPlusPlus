#pragma once

#include "services/interfaces/workflow/quake3/q3_mapselect_assets.hpp"

#include <string>

namespace sdl3cpp::services::impl {

/// Uppercases `s` (used to match Q3's all-caps map/bot-name conventions).
std::string ToUpper(std::string s);

/// Draws the levelshot image (loaded from the pk3 and cached in
/// `a.shots` on first use, keyed by the upper-cased map name) with its
/// red border, or a plain fallback rect if no image is available.
void DrawLevelshot(const Q3MapSelectAssets& a, const std::string& mapName,
                   float x, float y, float w, float h);

}  // namespace sdl3cpp::services::impl
