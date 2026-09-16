#include "services/interfaces/workflow/gta5/hud/gta5_hud.hpp"

#include "services/interfaces/workflow/gta5/hud/gta5_map_build.hpp"

#include <algorithm>
#include <cmath>

namespace sdl3cpp::services::impl {
namespace {

constexpr float kWide = 328.f, kTall = 200.f;  // the radar, in pixels
constexpr float kMetres = 2.5f;                // world metres a pixel

/// The six tiles, turned so the view points up, about the player at
/// the radar's centre; the scissor keeps them inside it.
void Tiles(Gta5MapFrame& frame, const Gta5MapLayout& l, const Gta5Hud& hud,
           glm::vec2 centre, glm::vec2 where, float heading) {
    const Gta5MapRect rect;
    const glm::vec2 me(rect.U(where.x), rect.V(where.y));
    const glm::vec2 pixels(rect.width / kMetres, rect.height / kMetres);
    const float c = std::cos(-heading), s = std::sin(-heading);
    const auto at = [&](glm::vec2 uv) {
        const glm::vec2 d = (uv - me) * pixels;
        return centre + glm::vec2(d.x * c - d.y * s, d.x * s + d.y * c);
    };
    const glm::vec2 cell(0.5f, 1.f / 3.f);
    for (int i = 0; i < 6; ++i) {
        const glm::vec2 t0 = glm::vec2(i % 2, i / 2) * cell;
        const glm::vec2 t1 = t0 + cell;
        AddGta5MapQuad(frame, l, hud.overlay.tiles[i], hud.overlay.sampler,
                       {at(t0), at({t1.x, t0.y}), at(t1), at({t0.x, t1.y})},
                       glm::vec4(0.f, 0.f, 1.f, 1.f));
    }
}

}  // namespace

void AddGta5HudRadar(Gta5MapFrame& frame, const Gta5MapLayout& l,
                     const Gta5Hud& hud, const Gta5HudState& s,
                     float height) {
    AddGta5HudBars(frame, l, hud, height, s);
    if (!hud.radar) return;
    const glm::vec2 lo(24.f, height - 46.f - kTall);
    const glm::vec2 hi = lo + glm::vec2(kWide, kTall);
    AddGta5MapRect(frame, l, hud.back, hud.overlay.sampler, lo - 3.f,
                   hi + 3.f);
    frame.clip = glm::vec4(lo, kWide, kTall);
    Tiles(frame, l, hud, (lo + hi) * 0.5f, s.where, s.heading);
    frame.clip = glm::vec4(0.f);
    // The map turns, so the arrow always points up, as GTA's.
    AddGta5MapTurned(frame, l, hud.overlay.marker, hud.overlay.sampler,
                     (lo + hi) * 0.5f, 9.f, 0.f);
}

}  // namespace sdl3cpp::services::impl
