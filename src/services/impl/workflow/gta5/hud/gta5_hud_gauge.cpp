#include "services/interfaces/workflow/gta5/hud/gta5_hud.hpp"

#include <algorithm>
#include <cmath>

namespace sdl3cpp::services::impl {
namespace {

constexpr float kPi = 3.14159265f;

/// Clockwise from straight up for a share of the dial's 270 degrees.
float Angle(float share) { return (share - 0.5f) * 1.5f * kPi; }

}  // namespace

void AddGta5HudGauge(Gta5MapFrame& frame, const Gta5MapLayout& layout,
                     const Gta5Hud& hud, SDL_GPUTexture* dial,
                     glm::vec2 centre, float radius, float value, float top,
                     float step, const std::string& caption) {
    AddGta5MapRect(frame, layout, dial, hud.overlay.sampler,
                   centre - radius, centre + radius);
    const float scale = std::max(1.f, std::round(radius / 55.f));
    for (float v = 0.f; v <= top + step * 0.01f; v += step) {
        const float a = Angle(v / top);
        const glm::vec2 at =
            centre + glm::vec2(std::sin(a), -std::cos(a)) * radius * 0.6f;
        AddGta5HudText(frame, layout, hud, at, scale,
                       std::to_string(static_cast<int>(v)), 0.f);
    }
    // In the open quarter at the bottom, clear of the end labels.
    AddGta5HudText(frame, layout, hud,
                   centre + glm::vec2(0.f, radius * 0.78f), scale, caption,
                   0.f);
    // A touch past the top at most, as a needle stops against its pin.
    const float share = std::clamp(value / top, 0.f, 1.04f);
    AddGta5MapTurned(frame, layout, hud.needle, hud.overlay.sampler, centre,
                     radius * 0.92f, Angle(share));
}

}  // namespace sdl3cpp::services::impl
