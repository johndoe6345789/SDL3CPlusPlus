#pragma once

#include <cstdint>
#include <map>
#include <string>
#include <vector>

namespace sdl3cpp::tools::fs2024 {

/// A Copernicus GLO-30 DEM tile: tiled, DEFLATE-compressed, float32,
/// WGS84 lon/lat. Internal tiles are decoded and cached on first use,
/// not all at once -- a bake only ever samples a small window of one.
class DemTile {
public:
    explicit DemTile(const std::string& path);

    /// Bilinear elevation in metres at (lon, lat). Throws
    /// std::runtime_error if the point falls outside the file.
    float Sample(double lon, double lat) const;

private:
    const std::vector<std::uint8_t>& DecodedTile(std::uint32_t tileIndex) const;
    float PixelAt(std::uint32_t row, std::uint32_t col) const;

    std::string path_;
    std::uint32_t width_ = 0, height_ = 0;
    std::uint32_t tileWidth_ = 0, tileLength_ = 0;
    std::vector<std::uint32_t> tileOffsets_, tileByteCounts_;
    double lon0_ = 0.0, lat0_ = 0.0, dlon_ = 1.0, dlat_ = 1.0;
    mutable std::map<std::uint32_t, std::vector<std::uint8_t>> cache_;
};

}  // namespace sdl3cpp::tools::fs2024
