#pragma once

#include <array>
#include <cstdint>
#include <vector>

namespace sdl3cpp::services::impl {

/// A decoded Episode I Racer texture: 64x64 RGBA8, row-major.
struct RacerTexture {
    std::uint32_t blockIndex = 0;
    int width = 64;
    int height = 64;
    std::vector<std::uint8_t> rgba;
};

/// Sixteen RGB565 colours from a 32-byte palette entry.
std::array<std::uint8_t, 16 * 4> DecodeRacerPalette(const std::uint8_t* raw);

/// Each 64x64 texture (2048 bytes, two 4-bit indices per byte, high nibble
/// first) is paired with the nearest 32-byte palette before it in block
/// order. Textures with no palette before them are skipped.
std::vector<RacerTexture> DecodeRacerTextures(
    const std::vector<std::uint8_t>& block);

}  // namespace sdl3cpp::services::impl
