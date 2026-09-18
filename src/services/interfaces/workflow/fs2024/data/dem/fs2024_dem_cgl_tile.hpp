#pragma once

#include <cstdint>
#include <vector>

namespace sdl3cpp::fs2024 {

/// One elevation tile of FS2024's own world DEM (`fs-base-cgl/CGL/<ddd>/
/// dem<ddd>.cgl`), as the raw samples its JPEG XR image stores: 257 x 257,
/// signed 16-bit, row 0 the tile's north edge, one sample of overlap with
/// each neighbour. What a sample means in metres is a separate step
/// (fs2024_dem_calibration.hpp) -- decoding and interpreting are kept
/// apart so the calibration can be tested against real terrain.
struct Fs2024DemSamples {
    int width = 0;
    int height = 0;
    std::vector<std::int16_t> raw;  ///< row-major, width * height

    std::int16_t At(int column, int row) const {
        return raw[static_cast<std::size_t>(row) * width + column];
    }
};

/// Decodes one decompressed dem tile: a 13-byte constant prefix
/// (`04 01 01 00 00 01 01 00 ...`), a u32 byte length, then a JPEG XR
/// image in its native BD16S grey format -- read straight through,
/// never converted: Windows' own WIC codec rescales that fixed-point
/// format on the way out and wraps it through the int16 range, which
/// is what made the heights look nonlinear when probed that way.
Fs2024DemSamples DecodeFs2024DemTile(const std::vector<std::uint8_t>& payload);

}  // namespace sdl3cpp::fs2024
