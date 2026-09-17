#pragma once

#include <cstdint>

namespace sdl3cpp::tools::fs2024 {

/// Decodes one 4x4 BC1 (DXT1) block into `out`, `outStride` bytes
/// between rows, RGBA8 (opaque; BC1's 1-bit alpha mode is not used by
/// any FS2024 texture seen so far). `block` is the compressed 8 bytes.
void DecodeBc1Block(const std::uint8_t* block, std::uint8_t* out,
                    int outStride);

/// Decodes one 4x4 BC3 (DXT5) block (8 bytes of interpolated alpha
/// then 8 bytes of BC1-style colour) into `out`, RGBA8.
void DecodeBc3Block(const std::uint8_t* block, std::uint8_t* out,
                    int outStride);

}  // namespace sdl3cpp::tools::fs2024
