#include "services/interfaces/workflow/racer/data/racer_texture.hpp"

#include "services/interfaces/workflow/racer/data/racer_block_table.hpp"

#include <optional>
#include <utility>

namespace sdl3cpp::services::impl {
namespace {

constexpr std::size_t kPaletteBytes = 32;
constexpr std::size_t kTextureBytes = 2048;

std::uint8_t Expand5(std::uint32_t v) {
    return static_cast<std::uint8_t>((v * 255) / 31);
}

}  // namespace

std::array<std::uint8_t, 16 * 4> DecodeRacerPalette(const std::uint8_t* raw) {
    // ARGB1555, big-endian: red bits 11-15, green 6-10, blue 1-5, and
    // alpha in bit 0. Layout per the swe1r-tools texture extractor.
    std::array<std::uint8_t, 16 * 4> out{};
    for (std::size_t i = 0; i < 16; ++i) {
        const std::uint32_t value = (raw[2 * i] << 8) | raw[2 * i + 1];
        out[4 * i + 0] = Expand5((value >> 11) & 31);
        out[4 * i + 1] = Expand5((value >> 6) & 31);
        out[4 * i + 2] = Expand5((value >> 1) & 31);
        out[4 * i + 3] = (value & 1) ? 255 : 0;
    }
    return out;
}

std::vector<RacerTexture> DecodeRacerTextures(
    const std::vector<std::uint8_t>& block) {
    std::vector<RacerTexture> textures;
    std::optional<std::array<std::uint8_t, 64>> palette;
    for (const RacerBlockEntry& entry : ReadRacerBlockTable(block)) {
        const std::size_t size = entry.end - entry.start;
        if (size == kPaletteBytes) {
            palette = DecodeRacerPalette(&block[entry.start]);
            continue;
        }
        if (size != kTextureBytes || !palette) continue;

        RacerTexture texture;
        texture.blockIndex = entry.index;
        texture.rgba.resize(64 * 64 * 4);
        for (std::size_t i = 0; i < kTextureBytes; ++i) {
            const std::uint8_t byte = block[entry.start + i];
            for (std::size_t nibble = 0; nibble < 2; ++nibble) {
                const std::uint8_t index =
                    nibble == 0 ? byte >> 4 : byte & 15;
                const std::size_t pixel = 2 * i + nibble;
                for (std::size_t c = 0; c < 4; ++c) {
                    texture.rgba[4 * pixel + c] = (*palette)[4 * index + c];
                }
            }
        }
        textures.push_back(std::move(texture));
    }
    return textures;
}

}  // namespace sdl3cpp::services::impl
