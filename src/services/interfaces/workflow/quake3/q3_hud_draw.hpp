#pragma once

#include "services/interfaces/workflow/quake3/q3_overlay_utils.hpp"

#include <SDL3/SDL_gpu.h>
#include <SDL3/SDL_render.h>

namespace sdl3cpp::services::impl {

/// Everything the status-bar HUD needs from context, gathered up front.
struct Q3HudAssets {
    SDL_Renderer* renderer  = nullptr;
    SDL_Texture* digits[11] = {};
    SDL_Texture* iconArmor  = nullptr;
    SDL_Texture* iconWeapon = nullptr;
    SDL_Texture* iconFace   = nullptr;
    /// Non-null when q3.hud_head_render already produced a GPU head
    /// portrait — the flat iconFace fallback is skipped in that case.
    SDL_GPUTexture* headGpuTex = nullptr;
    int health                 = 100;
    int armor                  = 0;
    int ammo                   = 50;
};

/// The face-rect position/size q3.hud_head_render's GPU blit step reads
/// to place its head portrait quad on the swapchain.
struct Q3HudFaceRect {
    float x, y, w, h;
};

/**
 * @brief Draws the ammo/health/armor status bar (ioq3 cg_draw.c layout,
 *        native 640x480).
 *
 * @return The face-rect the head portrait should be blitted into,
 *         regardless of whether the flat icon fallback was drawn.
 */
Q3HudFaceRect DrawQ3Hud(const Q3HudAssets& assets);

}  // namespace sdl3cpp::services::impl
