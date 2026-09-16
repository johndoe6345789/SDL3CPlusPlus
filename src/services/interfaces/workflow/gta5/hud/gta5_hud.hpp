#pragma once

#include "services/interfaces/i_logger.hpp"
#include "services/interfaces/workflow/gta5/hud/gta5_map_art.hpp"
#include "services/interfaces/workflow/gta5/hud/gta5_map_frame.hpp"
#include "services/interfaces/workflow/gta5/hud/gta5_map_overlay.hpp"
#include "services/interfaces/workflow/gta5/hud/gta5_hud_state.hpp"

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
    bool radar{false};  // the minimap tiles loaded
};
/// `tilesDir` holds the minimap tiles for the radar; without them the
/// HUD still draws, radar aside.
bool LoadGta5Hud(Gta5Hud& hud, SDL_GPUDevice* device,
                 SDL_GPUTextureFormat format, const std::string& tilesDir,
                 Gta5UploadBatch& uploads,
                 const std::shared_ptr<ILogger>& logger);

/// The needle, 64 x 64 RGBA.
std::vector<std::uint8_t> Gta5HudNeedlePixels();

Gta5MapFrame BuildGta5HudFrame(const Gta5Hud& hud, int width, int height,
                               const Gta5HudState& state);

}  // namespace sdl3cpp::services::impl

#include "services/interfaces/workflow/gta5/hud/gta5_hud_parts.hpp"
