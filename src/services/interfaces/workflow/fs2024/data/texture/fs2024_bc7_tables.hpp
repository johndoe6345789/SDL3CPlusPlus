#pragma once

#include <cstdint>

namespace sdl3cpp::fs2024 {

/// BC7's fixed partition shapes (BPTC spec, 64 per subset count), one
/// 2-bit subset number per texel (texel i at bits 2i, row-major), and
/// the texel whose index loses its top bit ("anchor") for subsets 1
/// and 2 -- subset 0's anchor is always texel 0.
extern const std::uint32_t kBc7Partition2[64];
extern const std::uint32_t kBc7Partition3[64];
extern const std::uint8_t kBc7Anchor2[64];
extern const std::uint8_t kBc7Anchor3a[64];
extern const std::uint8_t kBc7Anchor3b[64];

}  // namespace sdl3cpp::fs2024
