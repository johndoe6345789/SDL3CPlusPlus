#include "services/interfaces/workflow/gta5/gta5_hud.hpp"

#include <algorithm>
#include <cmath>

namespace sdl3cpp::services::impl {
namespace {

/// Health and armour, bottom left, where GTA puts them under its radar.
void Bars(Gta5MapFrame& frame, const Gta5MapLayout& l, const Gta5Hud& hud,
          float height, const Gta5HudState& s) {
    const float w = 160.f, h = 10.f;
    const glm::vec2 at(24.f, height - 36.f);
    SDL_GPUSampler* sampler = hud.overlay.sampler;
    AddGta5MapRect(frame, l, hud.back, sampler, at - 3.f,
                   at + glm::vec2(2.f * w + 8.f, h) + 3.f);
    AddGta5MapRect(frame, l, hud.health, sampler, at,
                   at + glm::vec2(w * std::clamp(s.health / 100.f, 0.f, 1.f),
                                  h));
    const glm::vec2 right = at + glm::vec2(w + 8.f, 0.f);
    AddGta5MapRect(frame, l, hud.armour, sampler, right,
                   right + glm::vec2(w * std::clamp(s.armour / 100.f, 0.f, 1.f),
                                     h));
}

}  // namespace

void AddGta5HudText(Gta5MapFrame& frame, const Gta5MapLayout& layout,
                    const Gta5Hud& hud, glm::vec2 at, float scale,
                    const std::string& text, float align) {
    const float width = (6.f * float(text.size()) - 1.f) * scale;
    const float shift = align > 0.5f ? width : width / 2.f;
    AddGta5MapText(frame, layout, hud.overlay,
                   at - glm::vec2(shift, align > 0.5f ? 0.f : 3.5f * scale),
                   scale, text);
}

Gta5MapFrame BuildGta5HudFrame(const Gta5Hud& hud, int width, int height,
                               const Gta5HudState& s) {
    Gta5MapFrame frame;
    const Gta5MapLayout l = FitGta5Map(width, height);
    const float w = float(width), h = float(height);
    Bars(frame, l, hud, h, s);
    // The weapon, top right under the clock: its name, then the clip and
    // what is left besides.
    if (!s.weapon.empty()) {
        AddGta5HudText(frame, l, hud, glm::vec2(w - 24.f, 36.f), 3.f,
                       s.weapon, 1.f);
        if (s.clip >= 0) {
            AddGta5HudText(frame, l, hud, glm::vec2(w - 24.f, 66.f), 3.f,
                           std::to_string(s.clip) + "  " +
                               std::to_string(s.reserve),
                           1.f);
        }
    }
    AddGta5HudMenu(frame, l, hud, s, w, h);
    AddGta5HudWheel(frame, l, hud, s, w, h);
    if (!s.driving) return frame;
    // Speed and revs, bottom right, as a dashboard's pair of dials.
    const float r = std::clamp(h * 0.14f, 70.f, 130.f);
    const glm::vec2 speedo(w - 24.f - r, h - 24.f - r);
    AddGta5HudGauge(frame, l, hud, hud.speedDial, speedo, r, s.kmh, 240.f,
                    40.f, std::to_string(int(s.kmh)) + " KMH");
    const float thousands = 0.8f + s.revs * 6.2f;  // idle 800 to 7000
    AddGta5HudGauge(frame, l, hud, hud.tachDial,
                    speedo - glm::vec2(2.15f * r, 0.f), r, thousands, 8.f, 1.f,
                    "GEAR " + std::to_string(s.gear));
    return frame;
}

}  // namespace sdl3cpp::services::impl
