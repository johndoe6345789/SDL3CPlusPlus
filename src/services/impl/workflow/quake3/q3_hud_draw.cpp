#include "services/interfaces/workflow/quake3/q3_hud_draw.hpp"

namespace sdl3cpp::services::impl {
namespace {

using q3overlay::DrawHudNumber;
using q3overlay::kH;

void DrawIcon(SDL_Renderer* r, SDL_Texture* t, float x, float y, float w,
             float h) {
    if (!t) return;
    SDL_SetTextureAlphaMod(t, 255);
    SDL_SetTextureColorMod(t, 255, 255, 255);
    SDL_SetTextureBlendMode(t, SDL_BLENDMODE_BLEND);
    SDL_FRect dst{x, y, w, h};
    SDL_RenderTexture(r, t, nullptr, &dst);
}

}  // namespace

Q3HudFaceRect DrawQ3Hud(const Q3HudAssets& a) {
    // Layout constants (match Q3A source, scaled to 640x360)
    constexpr float kScale = static_cast<float>(kH) / 480.f;  // 0.75
    constexpr float kCharW = 32.f;   // digit sprite width (native 32b)
    constexpr float kCharH = 32.f;   // drawn at native height
    constexpr float kIconSz = 48.f * kScale;  // 36px
    constexpr float kIconOff = kCharW * 3.f + 4.f;  // 100px (TEXT_ICON_SPACE=4)
    constexpr float kHudY = kH - kCharH - 2.f;  // 326px — digit baseline
    constexpr float kHeadSz = 48.f * kScale * 1.25f;  // 45px — ICON_SIZE*1.25
    constexpr float kHeadY = kH - kHeadSz - 2.f;

    // icon baseline (taller than digits)
    const float iconY = kH - kIconSz - 2.f;

    // Left cluster: ammo# + ammo/weapon icon — x=0
    DrawHudNumber(a.renderer, a.digits, 0.f, kHudY, a.ammo, 1.0f);
    DrawIcon(a.renderer, a.iconWeapon, kIconOff, iconY, kIconSz, kIconSz);

    // Center cluster: health# + head portrait — x=185
    DrawHudNumber(a.renderer, a.digits, 185.f, kHudY, a.health, 1.0f);
    // Head portrait drawn by q3.hud_head_render on GPU and blitted by
    // overlay.sw.end. Fall back to flat face icon if GPU head is
    // unavailable.
    if (!a.headGpuTex) {
        DrawIcon(a.renderer, a.iconFace, 185.f + kIconOff, kHeadY, kHeadSz,
                kHeadSz);
    }

    // Right cluster: armor# + armor icon — x=370
    if (a.armor > 0) {
        DrawHudNumber(a.renderer, a.digits, 370.f, kHudY, a.armor, 1.0f);
        DrawIcon(a.renderer, a.iconArmor, 370.f + kIconOff, iconY, kIconSz,
                kIconSz);
    }

    return {185.f + kIconOff, kHeadY, kHeadSz, kHeadSz};
}

}  // namespace sdl3cpp::services::impl
