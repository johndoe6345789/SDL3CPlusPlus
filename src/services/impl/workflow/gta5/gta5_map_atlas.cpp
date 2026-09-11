#include "services/interfaces/workflow/gta5/gta5_map_art.hpp"

#include "services/interfaces/workflow/gta5/gta5_map_font.hpp"

#include <cctype>
#include <cmath>
#include <cstring>

namespace sdl3cpp::services::impl {
namespace {

constexpr int kWidth = 256;
constexpr int kHeight = 48;
constexpr int kCell = 32;      // an icon's cell
constexpr int kGlyphTop = 32;  // the glyph row
constexpr char kChars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789-'.";

using Pixels = std::vector<std::uint8_t>;

void Put(Pixels& px, int x, int y, std::uint8_t r, std::uint8_t g,
         std::uint8_t b) {
    std::uint8_t* p = &px[(y * kWidth + x) * 4];
    p[0] = r, p[1] = g, p[2] = b, p[3] = 255;
}

void Letter(Pixels& px, char c, int left, int top, int scale) {
    const Gta5Glyph& glyph = Gta5MapGlyph(c);
    for (int dot = 0; dot < 35 * scale * scale; ++dot) {
        const int x = dot % (5 * scale), y = dot / (5 * scale);
        if ((glyph[y / scale] >> (4 - x / scale)) & 1) {
            Put(px, left + x, top + y, 255, 255, 255);
        }
    }
}

/// A disc in the category's colour, a dark rim, the letter in white.
void Icon(Pixels& px, int cell, const Gta5MapPoiCategory& category) {
    const float cx = cell * kCell + 16.f, cy = 16.f;
    for (int y = 0; y < kCell; ++y) {
        for (int x = cell * kCell; x < (cell + 1) * kCell; ++x) {
            const float d = std::hypot(x + 0.5f - cx, y + 0.5f - cy);
            const auto& c = category.colour;
            if (d <= 12.5f) Put(px, x, y, c[0], c[1], c[2]);
            else if (d <= 15.f) Put(px, x, y, 20, 20, 20);
        }
    }
    Letter(px, category.letter, cell * kCell + 11, 9, 2);
}

}  // namespace

bool CreateGta5MapAtlas(Gta5MapOverlay& map, SDL_GPUDevice* device,
                        Gta5UploadBatch& uploads) {
    Pixels px(kWidth * kHeight * 4, 0);
    // LoadGta5MapPois keeps at most kGta5MapMaxCategories: one row.
    for (std::size_t i = 0; i < map.pois.categories.size(); ++i) {
        Icon(px, static_cast<int>(i), map.pois.categories[i]);
    }
    for (int i = 0; kChars[i]; ++i) Letter(px, kChars[i], 6 * i, kGlyphTop, 1);
    map.atlas = CreateGta5MapRgba(device, kWidth, kHeight, px, uploads);
    return map.atlas != nullptr;
}

glm::vec4 Gta5MapIconUv(int category) {
    return {float(category * kCell) / kWidth, 0.f,
            float((category + 1) * kCell) / kWidth, float(kCell) / kHeight};
}

bool Gta5MapGlyphUv(char c, glm::vec4& uv) {
    const char* at =
        std::strchr(kChars, std::toupper(static_cast<unsigned char>(c)));
    if (!at || !*at) return false;  // blank, or the terminator
    const float left = 6.f * static_cast<float>(at - kChars);
    uv = {left / kWidth, float(kGlyphTop) / kHeight, (left + 5.f) / kWidth,
          (kGlyphTop + 7.f) / kHeight};
    return true;
}

}  // namespace sdl3cpp::services::impl
