#include "services/interfaces/workflow/quake3/q3_mapselect_draw.hpp"
#include "services/interfaces/workflow/quake3/q3_mapselect_info.hpp"
#include "services/interfaces/workflow/quake3/q3_mapselect_levelshot.hpp"
#include "services/interfaces/workflow/quake3/q3_mapselect_nav.hpp"

#include <algorithm>

namespace sdl3cpp::services::impl {

using q3overlay::DrawPropText;
using q3overlay::DrawQ3Text;
using q3overlay::kH;
using q3overlay::kW;

void DrawQ3MapSelectScreen(const Q3MapSelectAssets& a) {
    SDL_SetRenderDrawColor(a.renderer, 0, 0, 0, 255);
    SDL_FRect bg{0, 0, static_cast<float>(kW), static_cast<float>(kH)};
    SDL_RenderFillRect(a.renderer, &bg);

    constexpr float kCX = kW * 0.5f;
    DrawQ3Text(a.renderer, a.bigchars, kCX - 96.f, 12.f, "CHOOSE LEVEL",
               {200, 50, 20, 255}, 1.0f);

    const int nMaps = static_cast<int>(a.maps.size());
    const int idx =
        (nMaps > 0) ? std::max(0, std::min(a.selectedItem, nMaps - 1)) : 0;
    const std::string mapName =
        (nMaps > 0) ? a.maps[static_cast<size_t>(idx)].get<std::string>()
                    : "q3dm7";

    constexpr float kLW = 162.f, kLH = 162.f, kLX = (kW - kLW) * 0.5f,
                    kLY = 58.f;
    DrawLevelshot(a, mapName, kLX, kLY, kLW, kLH);

    const std::string mU = ToUpper(mapName);
    DrawPropText(a.renderer, a.prop, kCX, kLY + kLH + 6.f, mU.c_str(),
                 {255, 200, 50, 255}, 0.7f, true);
    DrawArenaInfo(a, mapName, kCX, kLY + kLH + 34.f);

    DrawNavArrows(a, idx, nMaps, kLY + kLH * 0.5f - 16.f);
    DrawMapSelectButtons(a);
}

}  // namespace sdl3cpp::services::impl
