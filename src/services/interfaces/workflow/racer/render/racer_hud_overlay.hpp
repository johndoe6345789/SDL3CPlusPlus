#pragma once

#include <SDL3/SDL_gpu.h>
#include <SDL3/SDL_render.h>
#include <SDL3/SDL_surface.h>

#include <string>

namespace sdl3cpp::services::impl {

/// HUD texture size: five lines of SDL's 8 px debug font, shown at 3x.
inline constexpr int kRacerHudWidth = 320;
inline constexpr int kRacerHudHeight = 54;
inline constexpr int kRacerHudScale = 3;

/// The racer's own HUD texture, text surface and screen quad. Drawn
/// with the engine's text-overlay pipeline, which takes any texture.
struct RacerHudOverlay {
    SDL_GPUDevice* device = nullptr;
    SDL_GPUTexture* texture = nullptr;
    SDL_GPUTransferBuffer* transfer = nullptr;
    SDL_GPUBuffer* quad = nullptr;
    SDL_Surface* surface = nullptr;
    SDL_Renderer* renderer = nullptr;
    bool quadUploaded = false;
};

bool CreateRacerHudOverlay(SDL_GPUDevice* device, RacerHudOverlay& out);
void DestroyRacerHudOverlay(RacerHudOverlay& hud);

/// Draws `text` (lines split on '\n') into the texture and uploads it,
/// plus the screen quad the first time. Outside any render pass.
void UploadRacerHud(RacerHudOverlay& hud, SDL_GPUCommandBuffer* cmd,
                    const std::string& text, int frameWidth,
                    int frameHeight);

/// Draws the HUD over the swapchain with `pipeline` and `sampler`.
void DrawRacerHud(const RacerHudOverlay& hud, SDL_GPUCommandBuffer* cmd,
                  SDL_GPUTexture* swapchain,
                  SDL_GPUGraphicsPipeline* pipeline, SDL_GPUSampler* sampler);

}  // namespace sdl3cpp::services::impl
