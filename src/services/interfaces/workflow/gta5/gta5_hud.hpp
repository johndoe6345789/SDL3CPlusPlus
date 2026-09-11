#pragma once

#include "services/interfaces/i_logger.hpp"
#include "services/interfaces/workflow/gta5/gta5_map_art.hpp"
#include "services/interfaces/workflow/gta5/gta5_map_frame.hpp"
#include "services/interfaces/workflow/gta5/gta5_map_overlay.hpp"

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace sdl3cpp::services::impl {

/// The heads-up display's GPU side: the map's quad overlay, its own
/// copy, with the gauges' art beside the lettering.
struct Gta5Hud {
    Gta5MapOverlay overlay;  // pipeline, buffers, samplers, lettering
    SDL_GPUTexture* speedDial{nullptr};
    SDL_GPUTexture* tachDial{nullptr};  // red from 6.5 of 8
    SDL_GPUTexture* needle{nullptr};
    SDL_GPUTexture* health{nullptr};  // one texel each: bar colours
    SDL_GPUTexture* armour{nullptr};
    SDL_GPUTexture* back{nullptr};
    bool tried{false};
    bool ready{false};
};

/// What the display shows this frame.
struct Gta5HudState {
    float health{100.f};  // 0..100
    float armour{0.f};    // 0..100
    std::string weapon;   // empty: nothing in hand, nothing shown
    int clip{0};          // -1: no ammunition (melee)
    int reserve{0};
    bool driving{false};
    float kmh{0.f};
    float revs{0.f};  // 0 idle .. 1 the redline
    int gear{1};
};

bool LoadGta5Hud(Gta5Hud& hud, SDL_GPUDevice* device,
                 SDL_GPUTextureFormat format, Gta5UploadBatch& uploads,
                 const std::shared_ptr<ILogger>& logger);

/// The dials' faces and the needle, drawn here rather than loaded.
bool CreateGta5HudArt(Gta5Hud& hud, SDL_GPUDevice* device,
                      Gta5UploadBatch& uploads);

/// The needle, 64 x 64 RGBA: from the hub straight up, over a grey hub.
std::vector<std::uint8_t> Gta5HudNeedlePixels();

Gta5MapFrame BuildGta5HudFrame(const Gta5Hud& hud, int width, int height,
                               const Gta5HudState& state);

/// A gauge `radius` pixels about `centre`: its face, labels every `step`
/// from 0 to `top`, `caption` below the hub, and the needle at `value`.
void AddGta5HudGauge(Gta5MapFrame& frame, const Gta5MapLayout& layout,
                     const Gta5Hud& hud, SDL_GPUTexture* dial,
                     glm::vec2 centre, float radius, float value, float top,
                     float step, const std::string& caption);

/// `text` centred on `at` (align 0), or ending at it (align 1).
void AddGta5HudText(Gta5MapFrame& frame, const Gta5MapLayout& layout,
                    const Gta5Hud& hud, glm::vec2 at, float scale,
                    const std::string& text, float align);

}  // namespace sdl3cpp::services::impl
