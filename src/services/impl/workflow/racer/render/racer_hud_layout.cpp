#include "services/interfaces/workflow/racer/render/racer_hud_step.hpp"

#include <sstream>

namespace sdl3cpp::services::impl {

RacerScreenRect RacerBannerRect(float frameWidth, float frameHeight) {
    RacerScreenRect r;
    const float halfWidth = kBannerScale * kBannerWidth / frameWidth;
    const float height = 2.f * kBannerScale * kBannerHeight / frameHeight;
    r.left = -halfWidth;
    r.right = halfWidth;
    r.top = 0.6f;
    r.bottom = r.top - height;
    return r;
}

RacerScreenRect RacerHudRect(float frameWidth, float frameHeight) {
    RacerScreenRect r;
    r.right = 1.f - 2.f * kHudMargin / frameWidth;
    r.left = r.right - 2.f * kHudScale * kHudWidth / frameWidth;
    r.top = 1.f - 2.f * kHudMargin / frameHeight;
    r.bottom = r.top - 2.f * kHudScale * kHudHeight / frameHeight;
    return r;
}

std::vector<RacerPanelLine> RacerHudLines(const std::string& text) {
    std::vector<RacerPanelLine> lines;
    std::istringstream in(text);
    std::string line;
    for (float y = 2.f; std::getline(in, line); y += 10.f) {
        lines.push_back({line, {255, 220, 50, 255}, 4.f, y});
    }
    return lines;
}

}  // namespace sdl3cpp::services::impl
