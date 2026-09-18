#pragma once

#include <cstdint>

namespace sdl3cpp::fs2024 {

/// Decodes one 16-byte BC7 (BPTC) block into `out`, RGBA8, all eight
/// modes. FS2024's building generator keeps every facade, roof and
/// window texture in BC7 texture arrays (bf-pgg/PGG/textures).
/// An invalid block (mode byte of zero) decodes to transparent black.
void DecodeBc7Block(const std::uint8_t* block, std::uint8_t* out,
                    int outStride);

}  // namespace sdl3cpp::fs2024
