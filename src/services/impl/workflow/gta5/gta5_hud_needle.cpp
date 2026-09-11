#include "services/interfaces/workflow/gta5/gta5_hud.hpp"

#include <cmath>

namespace sdl3cpp::services::impl {
namespace {

void Put(std::vector<std::uint8_t>& px, int size, int x, int y,
         std::uint8_t r, std::uint8_t g, std::uint8_t b) {
    std::uint8_t* p = &px[(y * size + x) * 4];
    p[0] = r, p[1] = g, p[2] = b, p[3] = 255;
}

}  // namespace

/// The needle: from the hub straight up to the rim, over a grey hub.
std::vector<std::uint8_t> Gta5HudNeedlePixels() {
    constexpr int n = 64;
    std::vector<std::uint8_t> px(n * n * 4, 0);
    for (int y = 0; y < n; ++y) {
        for (int x = 0; x < n; ++x) {
            const float dx = x + 0.5f - n / 2.f, dy = y + 0.5f - n / 2.f;
            if (dx * dx + dy * dy < 16.f) {
                Put(px, n, x, y, 150, 150, 155);
            } else if (std::abs(dx) < 1.6f && dy < 0.f && dy > -n / 2.f + 2) {
                Put(px, n, x, y, 255, 70, 30);
            }
        }
    }
    return px;
}

}  // namespace sdl3cpp::services::impl
