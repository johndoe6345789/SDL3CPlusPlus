#include "services/interfaces/workflow/gta5/hud/gta5_hud.hpp"

#include "services/interfaces/workflow/gta5/hud/gta5_map_build.hpp"

#include <algorithm>

namespace sdl3cpp::services::impl {
namespace {

constexpr float kWide = 328.f, kTall = 200.f;  // the radar, in pixels
constexpr float kMetres = 2.5f;                // world metres a pixel

/// The six tiles, cut to the window of the map about the player.
void Tiles(Gta5MapFrame& frame, const Gta5MapLayout& l, const Gta5Hud& hud,
           glm::vec2 lo, glm::vec2 where) {
    const Gta5MapRect rect;
    const glm::vec2 centre(rect.U(where.x), rect.V(where.y));
    const glm::vec2 half(kWide * kMetres / rect.width * 0.5f,
                         kTall * kMetres / rect.height * 0.5f);
    const glm::vec2 w0 = centre - half, w1 = centre + half;
    const glm::vec2 cell(0.5f, 1.f / 3.f), size(kWide, kTall);
    for (int i = 0; i < 6; ++i) {
        const glm::vec2 t0 = glm::vec2(i % 2, i / 2) * cell;
        const glm::vec2 i0 = glm::max(t0, w0);
        const glm::vec2 i1 = glm::min(t0 + cell, w1);
        if (i0.x >= i1.x || i0.y >= i1.y) continue;
        const glm::vec2 a = (i0 - t0) / cell, b = (i1 - t0) / cell;
        AddGta5MapRect(frame, l, hud.overlay.tiles[i], hud.overlay.sampler,
                       lo + (i0 - w0) / (w1 - w0) * size,
                       lo + (i1 - w0) / (w1 - w0) * size, glm::vec4(a, b));
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
    Tiles(frame, l, hud, lo, s.where);
    AddGta5MapTurned(frame, l, hud.overlay.marker, hud.overlay.sampler,
                     (lo + hi) * 0.5f, 9.f, s.heading);
}

}  // namespace sdl3cpp::services::impl
