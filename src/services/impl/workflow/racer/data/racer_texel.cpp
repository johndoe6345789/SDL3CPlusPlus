#include "racer_texel.hpp"

namespace sdl3cpp::services::impl {
namespace {

constexpr RacerRgba kMissing{255, 0, 255, 255};

}  // namespace



/// High nibble first, as the N64 stores 4-bit texels.
static int Nibble(const std::vector<std::uint8_t>& pixels,
                  std::size_t index) {
    const std::size_t byte = index / 2;
    if (byte >= pixels.size()) return -1;
    return index % 2 == 0 ? pixels[byte] >> 4 : pixels[byte] & 15;
}

RacerRgba RacerTexel(const std::vector<std::uint8_t>& pixels,
           const std::vector<RacerRgba>& palette, RacerTextureFormat format,
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

std::size_t RacerTexelsIn(std::size_t bytes, RacerTextureFormat format) {
    switch (format) {
    case RacerTextureFormat::Indexed4:
    case RacerTextureFormat::Intensity4:
        return 2 * bytes;
    case RacerTextureFormat::Rgba32:
        return bytes / 4;
    default:
        return bytes;
    }
}

}  // namespace sdl3cpp::services::impl
