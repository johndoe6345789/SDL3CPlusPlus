#include "services/interfaces/workflow/fs2024/prepare/texture/fs2024_bc_decode.hpp"

#include <cstring>

namespace sdl3cpp::tools::fs2024 {
namespace {

struct Rgb {
    std::uint8_t r = 0, g = 0, b = 0;
};

Rgb Unpack565(std::uint16_t v) {
    const std::uint8_t r5 = (v >> 11) & 0x1F, g6 = (v >> 5) & 0x3F,
                       b5 = v & 0x1F;
    return {static_cast<std::uint8_t>((r5 << 3) | (r5 >> 2)),
           static_cast<std::uint8_t>((g6 << 2) | (g6 >> 4)),
           static_cast<std::uint8_t>((b5 << 3) | (b5 >> 2))};
}

Rgb Lerp(const Rgb& a, const Rgb& b, int num, int den) {
    return {static_cast<std::uint8_t>((a.r * (den - num) + b.r * num) / den),
           static_cast<std::uint8_t>((a.g * (den - num) + b.g * num) / den),
           static_cast<std::uint8_t>((a.b * (den - num) + b.b * num) / den)};
}

/// The 8-byte BC1-style colour half shared by BC1 itself and BC3's
/// colour block (BC3 always uses the 4-colour interpolation, never
/// BC1's punch-through-alpha 3-colour mode).
void DecodeColorHalf(const std::uint8_t* block, Rgb colors[4],
                     bool fourColorOnly) {
    std::uint16_t c0 = 0, c1 = 0;
    std::memcpy(&c0, block, 2);
    std::memcpy(&c1, block + 2, 2);
    colors[0] = Unpack565(c0);
    colors[1] = Unpack565(c1);
    if (fourColorOnly || c0 > c1) {
        colors[2] = Lerp(colors[0], colors[1], 1, 3);
        colors[3] = Lerp(colors[0], colors[1], 2, 3);
    } else {
        colors[2] = Lerp(colors[0], colors[1], 1, 2);
        colors[3] = {0, 0, 0};
    }
}

}  // namespace

void DecodeBc1Block(const std::uint8_t* block, std::uint8_t* out,
                    int outStride) {
    Rgb colors[4];
    DecodeColorHalf(block, colors, false);
    std::uint32_t indices = 0;
    std::memcpy(&indices, block + 4, 4);
    for (int pix = 0; pix < 16; ++pix) {
        const int idx = (indices >> (pix * 2)) & 0x3;
        std::uint8_t* px = out + (pix / 4) * outStride + (pix % 4) * 4;
        px[0] = colors[idx].r;
        px[1] = colors[idx].g;
        px[2] = colors[idx].b;
        px[3] = 255;
    }
}

void DecodeBc3Block(const std::uint8_t* block, std::uint8_t* out,
                    int outStride) {
    const std::uint8_t a0 = block[0], a1 = block[1];
    std::uint64_t abits = 0;
    for (int i = 0; i < 6; ++i) {
        abits |= static_cast<std::uint64_t>(block[2 + i]) << (8 * i);
    }
    std::uint8_t alphas[8] = {a0, a1};
    if (a0 > a1) {
        for (int i = 1; i <= 6; ++i) {
            alphas[1 + i] =
                static_cast<std::uint8_t>(((7 - i) * a0 + i * a1) / 7);
        }
    } else {
        for (int i = 1; i <= 4; ++i) {
            alphas[1 + i] =
                static_cast<std::uint8_t>(((5 - i) * a0 + i * a1) / 5);
        }
        alphas[6] = 0;
        alphas[7] = 255;
    }

    Rgb colors[4];
    DecodeColorHalf(block + 8, colors, true);
    std::uint32_t indices = 0;
    std::memcpy(&indices, block + 12, 4);
    for (int pix = 0; pix < 16; ++pix) {
        const int cidx = (indices >> (pix * 2)) & 0x3;
        const int aidx = (abits >> (pix * 3)) & 0x7;
        std::uint8_t* px = out + (pix / 4) * outStride + (pix % 4) * 4;
        px[0] = colors[cidx].r;
        px[1] = colors[cidx].g;
        px[2] = colors[cidx].b;
        px[3] = alphas[aidx];
    }
}

}  // namespace sdl3cpp::tools::fs2024
