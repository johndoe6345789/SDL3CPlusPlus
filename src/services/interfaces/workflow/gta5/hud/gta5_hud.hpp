#pragma once

#include "services/interfaces/i_logger.hpp"
#include "services/interfaces/workflow/gta5/hud/gta5_map_art.hpp"
#include "services/interfaces/workflow/gta5/hud/gta5_map_frame.hpp"
#include "services/interfaces/workflow/gta5/hud/gta5_map_overlay.hpp"
#include "services/interfaces/workflow/gta5/hud/gta5_menu.hpp"

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace sdl3cpp::services::impl {
/// The display's GPU side: the map's quad overlay, its own copy.
struct Gta5Hud {
    Gta5MapOverlay overlay;  // pipeline, buffers, samplers, lettering
    SDL_GPUTexture* speedDial{nullptr};
    SDL_GPUTexture* tachDial{nullptr};
    SDL_GPUTexture* needle{nullptr};
    SDL_GPUTexture* health{nullptr};
    SDL_GPUTexture* armour{nullptr};
    SDL_GPUTexture* back{nullptr};
    // One texel each: bar and panel colours, and a white crosshair.
    SDL_GPUTexture* title{nullptr};
    SDL_GPUTexture* highlight{nullptr};
    SDL_GPUTexture* mark{nullptr};
    bool tried{false};
    bool ready{false};
};
struct Gta5HudState {
    float health{100.f};
    float armour{0.f};
    std::string weapon;   // empty: nothing in hand, nothing shown
    int clip{0};          // -1: no ammunition (melee)
    int reserve{0};
    bool driving{false};
    float kmh{0.f};
    float revs{0.f};  // 0 idle .. 1 redline
    int gear{1};
    std::string prompt;  // "E  LS CUSTOMS": a shop's door
    bool menuOpen{false};
    Gta5Menu menu;
    Gta5Wheel wheel;
};
bool LoadGta5Hud(Gta5Hud& hud, SDL_GPUDevice* device,
                 SDL_GPUTextureFormat format, Gta5UploadBatch& uploads,
                 const std::shared_ptr<ILogger>& logger);
/// The dials and needle, drawn not loaded.
bool CreateGta5HudArt(Gta5Hud& hud, SDL_GPUDevice* device,
                      Gta5UploadBatch& uploads);

/// The needle, 64 x 64 RGBA.
std::vector<std::uint8_t> Gta5HudNeedlePixels();

Gta5MapFrame BuildGta5HudFrame(const Gta5Hud& hud, int width, int height,
                               const Gta5HudState& state);

/// A gauge about `centre`: face, labels to `top`, needle at `value`.
void AddGta5HudGauge(Gta5MapFrame& frame, const Gta5MapLayout& layout,
                     const Gta5Hud& hud, SDL_GPUTexture* dial,
                     glm::vec2 centre, float radius, float value, float top,
                     float step, const std::string& caption);

/// The weapon wheel.
void AddGta5HudWheel(Gta5MapFrame& frame, const Gta5MapLayout& layout,
                     const Gta5Hud& hud, const Gta5HudState& state, float w,
                     float h);

/// A shop's prompt, or its open menu.
void AddGta5HudMenu(Gta5MapFrame& frame, const Gta5MapLayout& layout,
                    const Gta5Hud& hud, const Gta5HudState& state, float w,
                    float h);

/// `text` centred on `at` (align 0), or ending there (align 1).
void AddGta5HudText(Gta5MapFrame& frame, const Gta5MapLayout& layout,
                    const Gta5Hud& hud, glm::vec2 at, float scale,
                    const std::string& text, float align);

}  // namespace sdl3cpp::services::impl
