#pragma once

#include <SDL3/SDL_gpu.h>
#include <SDL3/SDL_pixels.h>
#include <SDL3/SDL_render.h>

#include <string>
#include <vector>

namespace sdl3cpp::services::impl {

/// A screen rectangle in normalised device coordinates (y up), and the
/// part of the panel's texture shown in it.
struct RacerScreenRect {
    float left = -1.f, top = 1.f, right = 1.f, bottom = -1.f;
    float u0 = 0.f, v0 = 0.f, u1 = 1.f, v1 = 1.f;
};

/// One line of text on a panel, in texture pixels (SDL's 8 px font).
/// With a box size and no text it is a filled box instead, drawn in
/// `colour` (its alpha too) behind the text that follows it.
struct RacerPanelLine {
    std::string text;
    SDL_Color colour{255, 220, 50, 255};
    float x = 0.f;
    float y = 0.f;
    float boxWidth = 0.f;
    float boxHeight = 0.f;
};

/// A texture shown on a screen rectangle with the engine's text-overlay
/// pipeline: either text drawn on the CPU (HUD, menus) or an image.
struct RacerPanel {
    SDL_GPUDevice* device = nullptr;
    SDL_GPUTexture* texture = nullptr;
    SDL_GPUSampler* sampler = nullptr;   ///< own sampler, images only
    SDL_GPUTransferBuffer* transfer = nullptr;
    SDL_GPUBuffer* quad = nullptr;
    SDL_Surface* surface = nullptr;      ///< text panels only
    SDL_Renderer* renderer = nullptr;
    int width = 0;
    int height = 0;
    bool quadUploaded = false;
};

bool CreateRacerTextPanel(SDL_GPUDevice* device, int width, int height,
                          RacerPanel& out);
void DestroyRacerPanel(RacerPanel& panel);

/// Redraws a text panel: clears to black at `shade` opacity, writes the
/// lines, and uploads the texture. Outside any render pass.
void UploadRacerPanelText(RacerPanel& panel, SDL_GPUCommandBuffer* cmd,
                          const std::vector<RacerPanelLine>& lines,
                          Uint8 shade);

/// Places the panel on screen. Outside any render pass.
void UploadRacerPanelRect(RacerPanel& panel, SDL_GPUCommandBuffer* cmd,
                          const RacerScreenRect& rect);

/// Draws over the swapchain. `sampler` is used unless the panel has its
/// own.
void DrawRacerPanel(const RacerPanel& panel, SDL_GPUCommandBuffer* cmd,
                    SDL_GPUTexture* swapchain,
                    SDL_GPUGraphicsPipeline* pipeline,
                    SDL_GPUSampler* sampler);

}  // namespace sdl3cpp::services::impl
