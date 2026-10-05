#include "services/interfaces/workflow/racer/data/racer_texture.hpp"

namespace sdl3cpp::services::impl {
namespace {

using Rgba = std::array<std::uint8_t, 4>;
constexpr Rgba kMissing{255, 0, 255, 255};

/// High nibble first, as the N64 stores 4-bit texels.
int Nibble(const std::vector<std::uint8_t>& pixels, std::size_t index) {
    const std::size_t byte = index / 2;
    if (byte >= pixels.size()) return -1;
    return index % 2 == 0 ? pixels[byte] >> 4 : pixels[byte] & 15;
}

Rgba Texel(const std::vector<std::uint8_t>& pixels,
           const std::vector<Rgba>& palette, RacerTextureFormat format,
           std::size_t index) {
    switch (format) {
    case RacerTextureFormat::Indexed4: {
        const int i = Nibble(pixels, index);
        return i >= 0 && i < static_cast<int>(palette.size()) ? palette[i]
                                                              : kMissing;
    }
    case RacerTextureFormat::Indexed8:
        if (index >= pixels.size() || pixels[index] >= palette.size()) {
            return kMissing;
        }
        return palette[pixels[index]];
    case RacerTextureFormat::Intensity4: {
        const int i = Nibble(pixels, index);
        if (i < 0) return kMissing;
        const auto v = static_cast<std::uint8_t>(i * 17);
        return {v, v, v, v};
    }
    case RacerTextureFormat::Intensity8: {
        if (index >= pixels.size()) return kMissing;
        const std::uint8_t v = pixels[index];
        return {v, v, v, v};
    }
    case RacerTextureFormat::Rgba32:
        if (4 * index + 3 >= pixels.size()) return kMissing;
        return {pixels[4 * index], pixels[4 * index + 1],
                pixels[4 * index + 2], pixels[4 * index + 3]};
    }
    return kMissing;
}

}  // namespace

RacerTexture DecodeRacerTexture(const std::vector<std::uint8_t>& pixels,
                                const std::vector<std::uint8_t>& palette,
                                RacerTextureFormat format, int width,
                                int height) {
    RacerTexture texture;
    if (width <= 0 || height <= 0) return texture;
    const std::size_t entries =
        format == RacerTextureFormat::Indexed8 ? 256 : 16;
    const auto colours = DecodeRacerPalette(palette, entries);
    texture.width = width;
    texture.height = height;
    const std::size_t count = static_cast<std::size_t>(width) * height;
    texture.rgba.resize(4 * count);
    for (std::size_t i = 0; i < count; ++i) {
        const Rgba c = Texel(pixels, colours, format, i);
        for (std::size_t k = 0; k < 4; ++k) texture.rgba[4 * i + k] = c[k];
    }
    return texture;
}

}  // namespace sdl3cpp::services::impl
