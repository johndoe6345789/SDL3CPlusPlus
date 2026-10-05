#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

namespace sdl3cpp::services::impl {

/// One part of a block item, as byte offsets into the block file. An
/// absent part (stored offset 0, such as a texture with no palette) has
/// `present` false and an empty span.
struct RacerBlockPart {
    std::size_t start = 0;
    std::size_t end = 0;
    bool present = false;
};

/// Number of parts per item in each block file.
constexpr std::size_t kRacerModelParts = 2;    // mask, data
constexpr std::size_t kRacerTextureParts = 2;  // pixels, palette
constexpr std::size_t kRacerSplineParts = 1;
constexpr std::size_t kRacerSpriteParts = 1;

/**
 * Reads a block's item table: a big-endian u32 item count, one u32
 * offset per part per item, then a u32 total size. A part ends where
 * the next present part (of this item or the next) starts. Returns an
 * empty list for a truncated or inconsistent table.
 */
std::vector<std::vector<RacerBlockPart>> ReadRacerBlock(
    const std::vector<std::uint8_t>& block, std::size_t partsPerItem);

/// Copies one part out of the block.
std::vector<std::uint8_t> RacerPartBytes(
    const std::vector<std::uint8_t>& block, const RacerBlockPart& part);

}  // namespace sdl3cpp::services::impl
