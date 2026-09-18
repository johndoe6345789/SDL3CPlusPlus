#pragma once

#include <cstdint>
#include <vector>

namespace sdl3cpp::fs2024 {

/// One tile of FS2024's ground-cover layer (`lcg<ddd>.cgl`): a small
/// lossless WebP image, 259 x 259 (a 257 core plus a one-sample skirt
/// each side) at levels 8-12 -- about 24 m a sample at London. RGBA8,
/// row-major, row 0 the tile's north edge.
struct Fs2024LcgImage {
    int width = 0;
    int height = 0;
    std::vector<std::uint8_t> rgba;

    const std::uint8_t* At(int column, int row) const {
        return rgba.data() +
               (static_cast<std::size_t>(row) * width + column) * 4;
    }
};

/// Decodes one decompressed lcg tile, which is a complete WebP file.
Fs2024LcgImage DecodeFs2024LcgTile(const std::vector<std::uint8_t>& payload);

}  // namespace sdl3cpp::fs2024
