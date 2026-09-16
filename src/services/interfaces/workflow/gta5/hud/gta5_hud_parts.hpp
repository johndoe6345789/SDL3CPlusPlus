#pragma once

// The pieces BuildGta5HudFrame assembles. Included by gta5_hud.hpp.

#include "services/interfaces/workflow/gta5/hud/gta5_hud.hpp"

#include <string>

namespace sdl3cpp::services::impl {

/// The radar, bottom left, and health and armour under it.
void AddGta5HudRadar(Gta5MapFrame& frame, const Gta5MapLayout& layout,
                     const Gta5Hud& hud, const Gta5HudState& state,
                     float height);
/// The dials and needle, drawn not loaded.
bool CreateGta5HudArt(Gta5Hud& hud, SDL_GPUDevice* device,
                      Gta5UploadBatch& uploads);

/// Health and armour, under the radar.
void AddGta5HudBars(Gta5MapFrame& frame, const Gta5MapLayout& layout,
                    const Gta5Hud& hud, float height,
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
