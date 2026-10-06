#include "services/interfaces/workflow/racer/data/racer_texture.hpp"

#include "racer_texel.hpp"

namespace sdl3cpp::services::impl {
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
    // A few textures hold fewer bytes than their size needs; their
    // missing rows repeat the rows present (hardware read on into
    // whatever followed), so only an empty texture shows magenta.
    const std::size_t present = RacerTexelsIn(pixels.size(), format);
    for (std::size_t i = 0; i < count; ++i) {
        const std::size_t source = present > 0 ? i % present : i;
        const auto c = RacerTexel(pixels, colours, format, source);
        for (std::size_t k = 0; k < 4; ++k) texture.rgba[4 * i + k] = c[k];
    }
    return texture;
}

}  // namespace sdl3cpp::services::impl
