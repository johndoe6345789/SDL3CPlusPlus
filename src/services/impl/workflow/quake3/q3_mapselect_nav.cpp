#include "services/interfaces/workflow/quake3/q3_mapselect_nav.hpp"

namespace sdl3cpp::services::impl {

using q3overlay::kW;

void DrawNavArrows(const Q3MapSelectAssets& a, int idx, int nMaps, float y) {
    if (a.arrowLeft && idx > 0) {
        SDL_FRect d{8.f, y, 64.f, 32.f};
        SDL_RenderTexture(a.renderer, a.arrowLeft, nullptr, &d);
    }
    if (a.arrowRight && idx < nMaps - 1) {
        SDL_FRect d{kW - 72.f, y, 64.f, 32.f};
        SDL_RenderTexture(a.renderer, a.arrowRight, nullptr, &d);
    }
}

void DrawMapSelectButtons(const Q3MapSelectAssets& a) {
    constexpr float kBY = static_cast<float>(q3overlay::kH) - 52.f,
                    kBW = 128.f, kBH = 46.f;
    auto drawBtn        = [&](SDL_Texture* t, float x) {
        if (!t) return;
        SDL_SetTextureAlphaMod(t, 255);
        SDL_SetTextureBlendMode(t, SDL_BLENDMODE_BLEND);
        SDL_FRect d{x, kBY, kBW, kBH};
        SDL_RenderTexture(a.renderer, t, nullptr, &d);
    };
    drawBtn(a.btnBack, kW * 0.12f - kBW * 0.5f);
    drawBtn(a.btnSkirmish, kW * 0.50f - kBW * 0.5f);
    drawBtn(a.btnFight, kW * 0.88f - kBW * 0.5f);
}

}  // namespace sdl3cpp::services::impl
