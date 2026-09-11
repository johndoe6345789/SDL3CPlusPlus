#include "services/interfaces/workflow/gta5/gta5_hud.hpp"

#include <cmath>
#include <cstdint>
#include <vector>

namespace sdl3cpp::services::impl {
namespace {

using Pixels = std::vector<std::uint8_t>;
constexpr int kDial = 256;
constexpr float kPi = 3.14159265f;
constexpr float kSweep = 1.5f * kPi;  // 270 degrees, from 7:30 to 4:30

void Put(Pixels& px, int size, int x, int y, std::uint8_t r, std::uint8_t g,
         std::uint8_t b, std::uint8_t a) {
    std::uint8_t* p = &px[(y * size + x) * 4];
    p[0] = r, p[1] = g, p[2] = b, p[3] = a;
}

/// A gauge's face: a dark disc and rim, `ticks` marks round the sweep --
/// every `major`th long -- and red past `red` (a share of the sweep).
Pixels Dial(int ticks, int major, float red) {
    Pixels px(kDial * kDial * 4, 0);
    const float c = kDial / 2.f, radius = c - 2.f;
    for (int y = 0; y < kDial; ++y) {
        for (int x = 0; x < kDial; ++x) {
            const float dx = x + 0.5f - c, dy = y + 0.5f - c;
            const float r = std::sqrt(dx * dx + dy * dy);
            if (r > radius) continue;
            // Clockwise from straight up, and how far round the sweep.
            const float share =
                (std::atan2(dx, -dy) + kSweep / 2.f) / kSweep;
            Put(px, kDial, x, y, 12, 14, 18, 190);
            if (r > radius - 4.f) Put(px, kDial, x, y, 200, 200, 205, 255);
            if (share < 0.f || share > 1.f) continue;
            if (share >= red && r > radius * 0.86f && r < radius - 5.f) {
                Put(px, kDial, x, y, 220, 40, 30, 255);
            }
            const float t = share * float(ticks - 1);
            const float mark = std::round(t);
            const bool big = int(mark) % major == 0;
            const float arc = std::abs(t - mark) * kSweep / (ticks - 1) * r;
            const float inner = radius * (big ? 0.76f : 0.86f);
            if (arc < (big ? 2.f : 1.2f) && r > inner && r < radius - 5.f) {
                Put(px, kDial, x, y, 235, 235, 235, 255);
            }
        }
    }
    return px;
}

}  // namespace

bool CreateGta5HudArt(Gta5Hud& hud, SDL_GPUDevice* device,
                      Gta5UploadBatch& uploads) {
    // Speed: 0 to 240 km/h, a label each 40, a mark each 10.
    hud.speedDial =
        CreateGta5MapRgba(device, kDial, kDial, Dial(25, 4, 2.f), uploads);
    // Revs: 0 to 8 thousand, a mark each 500, red from 6500.
    hud.tachDial = CreateGta5MapRgba(device, kDial, kDial,
                                     Dial(17, 2, 6.5f / 8.f), uploads);
    hud.needle =
        CreateGta5MapRgba(device, 64, 64, Gta5HudNeedlePixels(), uploads);
    hud.health = CreateGta5MapRgba(device, 1, 1, {90, 200, 90, 235}, uploads);
    hud.armour = CreateGta5MapRgba(device, 1, 1, {80, 160, 255, 235}, uploads);
    hud.back = CreateGta5MapRgba(device, 1, 1, {0, 0, 0, 150}, uploads);
    return hud.speedDial && hud.tachDial && hud.needle && hud.health &&
           hud.armour && hud.back;
}

}  // namespace sdl3cpp::services::impl
