#include "services/interfaces/workflow/gta5/hud/gta5_hud.hpp"

#include <algorithm>

namespace sdl3cpp::services::impl {

/// Health and armour, bottom left, where GTA puts them under its radar.
void AddGta5HudBars(Gta5MapFrame& frame, const Gta5MapLayout& l,
                    const Gta5Hud& hud, float height,
                    const Gta5HudState& s) {
    const float w = 160.f, h = 10.f;
    const glm::vec2 at(24.f, height - 36.f);
    SDL_GPUSampler* sampler = hud.overlay.sampler;
    AddGta5MapRect(frame, l, hud.back, sampler, at - 3.f,
                   at + glm::vec2(2.f * w + 8.f, h) + 3.f);
    AddGta5MapRect(
        frame, l, hud.health, sampler, at,
        at + glm::vec2(w * std::clamp(s.health / 100.f, 0.f, 1.f), h));
    const glm::vec2 right = at + glm::vec2(w + 8.f, 0.f);
    AddGta5MapRect(
        frame, l, hud.armour, sampler, right,
        right + glm::vec2(w * std::clamp(s.armour / 100.f, 0.f, 1.f), h));
}

}  // namespace sdl3cpp::services::impl
