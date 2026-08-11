#include "services/interfaces/workflow/quake3/workflow_q3_hud_step.hpp"
#include "services/interfaces/workflow/quake3/q3_overlay_utils.hpp"
#include <SDL3/SDL_render.h>
#include <SDL3/SDL_gpu.h>

// ─────────────────────────────────────────────────────────────────────────────
// Real Q3A status-bar layout (from ioq3 cg_draw.c) at native 640×480.
//
//  CHAR_WIDTH=32  CHAR_HEIGHT=48  ICON_SIZE=48  TEXT_ICON_SPACE=4
//  ammo    x=0,   y=432  (field width 3)
//  ammo icon  x=CHAR_WIDTH*3+TEXT_ICON_SPACE = 100
//  health  x=185, y=432  (field width 3)
//  head    x=285, y=420, size=60  (480-ICON_SIZE*1.25)
//  armor   x=370, y=432  (field width 3)
//  armor icon x=370+100 = 470
//
//  kScale = kH/480 = 1.0 — all sizes are exact ioq3 pixels.
// ─────────────────────────────────────────────────────────────────────────────

namespace sdl3cpp::services::impl {
using namespace q3overlay;

WorkflowQ3HudStep::WorkflowQ3HudStep(std::shared_ptr<ILogger> l) : logger_(std::move(l)) {}
std::string WorkflowQ3HudStep::GetPluginId() const { return "q3.hud"; }

void WorkflowQ3HudStep::Execute(
        const WorkflowStepDefinition&, WorkflowContext& context) {
    if (!context.GetBool("overlay.ready", false)) return;
    if (context.GetBool("q3.menu_open", false)) return;

    auto* r = context.Get<SDL_Renderer*>("overlay.renderer", nullptr);
    if (!r) return;

    SDL_Texture* digits[11] = {};
    for (int i = 0; i < 11; ++i)
        digits[i] = context.Get<SDL_Texture*>("overlay.tex.num." + std::to_string(i), nullptr);

    auto* iArmor  = context.Get<SDL_Texture*>("overlay.tex.icon_armor",  nullptr);
    auto* iWeapon = context.Get<SDL_Texture*>("overlay.tex.icon_weapon", nullptr);

    const int health = context.Get<int>("q3.player_health", 100);
    const int armor  = context.Get<int>("q3.player_armor",  0);
    const int ammo   = context.Get<int>("q3.player_ammo",   50);

    // ── Layout constants (match Q3A source, scaled to 640×360) ──────────────
    constexpr float kScale   = static_cast<float>(kH) / 480.f;  // 0.75
    constexpr float kCharW   = 32.f;          // digit sprite width (native 32b)
    constexpr float kCharH   = 32.f;          // drawn at native height
    constexpr float kIconSz  = 48.f * kScale; // 36px
    constexpr float kIconOff = kCharW * 3.f + 4.f; // 100px (TEXT_ICON_SPACE=4)
    constexpr float kHudY    = kH - kCharH - 2.f;   // 326px — digit baseline

    auto drawIcon = [&](SDL_Texture* t, float x, float y, float w, float h) {
        if (!t) return;
        SDL_SetTextureAlphaMod(t, 255); SDL_SetTextureColorMod(t, 255, 255, 255);
        SDL_SetTextureBlendMode(t, SDL_BLENDMODE_BLEND);
        SDL_FRect dst{x, y, w, h}; SDL_RenderTexture(r, t, nullptr, &dst);
    };

    const float iconY = kH - kIconSz - 2.f;  // icon baseline (taller than digits)

    // ── Left cluster: ammo# + ammo/weapon icon ── x=0 ───────────────────────
    DrawHudNumber(r, digits, 0.f, kHudY, ammo, 1.0f);
    drawIcon(iWeapon, kIconOff, iconY, kIconSz, kIconSz);

    // ── Center cluster: health# + head portrait ── x=185 ────────────────────
    DrawHudNumber(r, digits, 185.f, kHudY, health, 1.0f);
    // Head portrait drawn by q3.hud_head_render on GPU and blitted by overlay.sw.end.
    // Fall back to flat face icon if GPU head is unavailable.
    if (!context.Get<SDL_GPUTexture*>("overlay.head_gpu_tex", nullptr)) {
        auto* iFace = context.Get<SDL_Texture*>("overlay.tex.icon_face", nullptr);
        constexpr float kHeadSz = 48.f * kScale * 1.25f; // 45px — matches Q3A ICON_SIZE*1.25
        constexpr float kHeadY  = kH - kHeadSz - 2.f;
        drawIcon(iFace, 185.f + kIconOff, kHeadY, kHeadSz, kHeadSz);
    }

    // ── Right cluster: armor# + armor icon ── x=370 ─────────────────────────
    if (armor > 0) {
        DrawHudNumber(r, digits, 370.f, kHudY, armor, 1.0f);
        drawIcon(iArmor, 370.f + kIconOff, iconY, kIconSz, kIconSz);
    }

    // ── Store head portrait position for the GPU blit step ───────────────────
    // q3.hud_head_render reads these to position its quad on the swapchain.
    constexpr float kHeadSz = 48.f * kScale * 1.25f;
    constexpr float kHeadY  = kH - kHeadSz - 2.f;
    context.Set<float>("hud.face_rect_x", 185.f + kIconOff);
    context.Set<float>("hud.face_rect_y", kHeadY);
    context.Set<float>("hud.face_rect_w", kHeadSz);
    context.Set<float>("hud.face_rect_h", kHeadSz);
}
} // namespace sdl3cpp::services::impl
