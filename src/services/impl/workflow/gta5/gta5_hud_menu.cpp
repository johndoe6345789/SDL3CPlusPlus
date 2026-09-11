#include "services/interfaces/workflow/gta5/gta5_hud.hpp"

#include <algorithm>

namespace sdl3cpp::services::impl {
namespace {

constexpr int kShown = 12;  // rows at once; the list scrolls past that

void Prompt(Gta5MapFrame& frame, const Gta5MapLayout& l, const Gta5Hud& hud,
            float w, float h, const std::string& prompt) {
    const std::string text = "PRESS " + prompt;
    const float half = (6.f * float(text.size()) * 3.f) / 2.f + 14.f;
    const glm::vec2 at(w / 2.f, h - 110.f);
    AddGta5MapRect(frame, l, hud.back, hud.overlay.sampler,
                   at - glm::vec2(half, 22.f), at + glm::vec2(half, 22.f));
    AddGta5HudText(frame, l, hud, at, 3.f, text, 0.f);
}

}  // namespace

void AddGta5HudMenu(Gta5MapFrame& frame, const Gta5MapLayout& l,
                    const Gta5Hud& hud, const Gta5HudState& s, float w,
                    float h) {
    if (!s.menuOpen) {
        if (!s.prompt.empty()) Prompt(frame, l, hud, w, h, s.prompt);
        return;
    }
    const Gta5Menu& m = s.menu;
    const float x = 40.f, y = 110.f, width = 480.f, row = 32.f;
    SDL_GPUSampler* sampler = hud.overlay.sampler;
    const int count = static_cast<int>(m.items.size());
    const int shown = std::min(count, kShown);
    const int first =
        std::clamp(m.selected - shown / 2, 0, std::max(0, count - shown));
    // A title bar in the shop's colours, then the list on dark glass.
    AddGta5MapRect(frame, l, hud.title, sampler, glm::vec2(x, y),
                   glm::vec2(x + width, y + 52.f));
    AddGta5MapText(frame, l, hud.overlay, glm::vec2(x + 18.f, y + 15.f), 3.f,
                   m.title);
    const float top = y + 52.f;
    const float bottom = top + 12.f + shown * row + 34.f;
    AddGta5MapRect(frame, l, hud.back, sampler, glm::vec2(x, top),
                   glm::vec2(x + width, bottom));
    for (int i = 0; i < shown; ++i) {
        const int item = first + i;
        const float ry = top + 8.f + i * row;
        if (item == m.selected) {
            AddGta5MapRect(frame, l, hud.highlight, sampler,
                           glm::vec2(x + 6.f, ry), glm::vec2(x + width - 6.f,
                                                             ry + row - 4.f));
        }
        AddGta5MapText(frame, l, hud.overlay, glm::vec2(x + 18.f, ry + 7.f),
                       2.f, m.items[item]);
    }
    std::string hint = m.hint;
    if (count > shown) {
        hint += "   " + std::to_string(m.selected + 1) + " OF " +
                std::to_string(count);
    }
    AddGta5MapText(frame, l, hud.overlay, glm::vec2(x + 18.f, bottom - 26.f),
                   2.f, hint);
}

}  // namespace sdl3cpp::services::impl
