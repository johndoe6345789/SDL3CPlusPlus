#include "services/interfaces/workflow/gta5/hud/gta5_hud.hpp"

#include <algorithm>
#include <cmath>

namespace sdl3cpp::services::impl {

void AddGta5HudWheel(Gta5MapFrame& frame, const Gta5MapLayout& layout,
                     const Gta5Hud& hud, const Gta5HudState& s, float w,
                     float h) {
    const Gta5Wheel& wheel = s.wheel;
    if (!wheel.open || wheel.items.empty()) return;
    SDL_GPUSampler* sampler = hud.overlay.sampler;
    const glm::vec2 centre(w * 0.5f, h * 0.5f);
    const float radius = std::min(w, h) * 0.22f;
    // The ground it sits on, then the names round it, the one under the
    // hand picked out, and that name again in the middle.
    AddGta5MapRect(frame, layout, hud.back, sampler,
                   centre - radius * 1.5f, centre + radius * 1.5f);
    const auto count = static_cast<float>(wheel.items.size());
    for (std::size_t i = 0; i < wheel.items.size(); ++i) {
        const float turn = 6.2831853f * static_cast<float>(i) / count;
        const glm::vec2 at =
            centre + glm::vec2(std::sin(turn), -std::cos(turn)) * radius;
        const bool chosen = static_cast<int>(i) == wheel.selected;
        if (chosen) {
            const float half =
                3.f * static_cast<float>(wheel.items[i].size()) + 10.f;
            AddGta5MapRect(frame, layout, hud.highlight, sampler,
                           at - glm::vec2(half, 12.f),
                           at + glm::vec2(half, 12.f));
        }
        AddGta5HudText(frame, layout, hud, at, chosen ? 2.f : 2.f,
                       wheel.items[i], 0.f);
    }
    const int pick =
        std::clamp(wheel.selected, 0, static_cast<int>(count) - 1);
    AddGta5HudText(frame, layout, hud, centre, 3.f, wheel.items[pick], 0.f);
}

}  // namespace sdl3cpp::services::impl
