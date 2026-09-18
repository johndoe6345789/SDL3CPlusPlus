#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace sdl3cpp::fs2024 {

/// One decoded IFD (Image File Directory) entry, kept generic: this
/// reads whichever tags a GeoTIFF DEM happens to need, not the whole
/// TIFF tag space.
struct TiffTag {
    std::uint16_t id = 0;
    std::uint16_t type = 0;
    std::uint32_t count = 0;
    std::vector<std::uint8_t> raw;  ///< the tag's value bytes, as read

    std::uint32_t AsUint32(std::size_t index = 0) const;
    double AsDouble(std::size_t index = 0) const;
    std::vector<std::uint32_t> AsUint32Array() const;
};

/// The handful of tags a tiled, DEFLATE-compressed, float32 GeoTIFF
/// (Copernicus GLO-30's own format) needs to be read back.
struct TiffDirectory {
    std::uint32_t width = 0;
    std::uint32_t height = 0;
    std::uint32_t tileWidth = 0;
    std::uint32_t tileLength = 0;
    std::uint16_t compression = 0;
    std::uint16_t predictor = 1;
    std::uint16_t sampleFormat = 1;
    std::uint16_t bitsPerSample = 0;
    std::vector<std::uint32_t> tileOffsets;
    std::vector<std::uint32_t> tileByteCounts;
    double pixelScaleX = 1.0;
    double pixelScaleY = 1.0;
    double tiepointLon = 0.0;
    double tiepointLat = 0.0;
};

/// Reads `path`'s TIFF header and first IFD. Throws std::runtime_error
/// (naming the file) if it is not a TIFF, or is missing a tile layout
/// (only tiled images are supported -- Copernicus GLO-30 ships tiled).
TiffDirectory ReadTiffDirectory(const std::string& path);

}  // namespace sdl3cpp::fs2024
