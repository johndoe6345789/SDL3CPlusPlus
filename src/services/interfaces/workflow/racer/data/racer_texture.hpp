#pragma once

#include <array>
#include <cstdint>
#include <vector>

namespace sdl3cpp::services::impl {

/// Pixel formats named by a model's material (`MaterialTexture` +0x0C).
/// The texture block itself stores no format or size.
enum class RacerTextureFormat : std::uint16_t {
    Rgba32 = 0x0003,
    Indexed4 = 0x0200,       ///< 4-bit indices into an ARGB1555 palette.
    Indexed8 = 0x0201,       ///< 8-bit indices into an ARGB1555 palette.
    Intensity4 = 0x0400,     ///< 4-bit grey, alpha equal to grey.
    Intensity8 = 0x0401,     ///< 8-bit grey, alpha equal to grey.
};

/// A decoded texture: RGBA8, row-major, top row first.
struct RacerTexture {
    int width = 0;
    int height = 0;
    std::vector<std::uint8_t> rgba;
};

/// Sixteen or 256 big-endian ARGB1555 colours: red bits 11-15, green
/// 6-10, blue 1-5, alpha bit 0. `count` entries are read.
std::vector<std::array<std::uint8_t, 4>> DecodeRacerPalette(
    const std::vector<std::uint8_t>& raw, std::size_t count);

/// Decodes one texture. Indexed formats need `palette`; the others
/// ignore it. Pixels missing from a short buffer come out magenta, so
/// a size mismatch is visible rather than silent.
RacerTexture DecodeRacerTexture(const std::vector<std::uint8_t>& pixels,
                                const std::vector<std::uint8_t>& palette,
                                RacerTextureFormat format, int width,
                                int height);

}  // namespace sdl3cpp::services::impl
