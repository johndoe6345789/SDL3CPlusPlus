#include "services/interfaces/workflow/gta5/gta5_hud_cross.hpp"

#include <algorithm>

namespace sdl3cpp::services::impl {

void AddGta5HudCrosshair(Gta5MapFrame& frame, const Gta5MapLayout& layout,
                         const Gta5Hud& hud, const Gta5HudState& state,
                         float w, float h) {
    // Only with something in hand, and not behind a windscreen, a menu
    // or the wheel.
    if (state.weapon.empty() || state.driving || state.menuOpen) return;
    if (!state.wheel.items.empty()) return;
    const glm::vec2 mid(w * 0.5f, h * 0.5f);
    const float gap = 5.f, arm = 11.f, thick = 2.f;
    SDL_GPUSampler* sampler = hud.overlay.sampler;
    for (int i = 0; i < 4; ++i) {
        const bool flat = i < 2;  // left and right, then up and down
        const float sign = (i % 2) ? 1.f : -1.f;
        const glm::vec2 along =
            flat ? glm::vec2(1.f, 0.f) : glm::vec2(0.f, 1.f);
        const glm::vec2 wide = flat ? glm::vec2(0.f, thick * 0.5f)
                                    : glm::vec2(thick * 0.5f, 0.f);
        const glm::vec2 from = mid + along * (sign * gap);
        const glm::vec2 to = mid + along * (sign * (gap + arm));
        const glm::vec2 lo = glm::min(from, to) - wide;
        const glm::vec2 hi = glm::max(from, to) + wide;
        // A darker bar under each, so it reads against a bright sky as
        // well as against the road.
        AddGta5MapRect(frame, layout, hud.back, sampler, lo - 1.f, hi + 1.f);
        AddGta5MapRect(frame, layout, hud.mark, sampler, lo, hi);
    }
}

}  // namespace sdl3cpp::services::impl
