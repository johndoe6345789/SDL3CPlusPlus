#include "services/interfaces/workflow/fs2024/prepare/fs2024_dem_tile.hpp"

#include "services/interfaces/workflow/fs2024/prepare/fs2024_inflate.hpp"
#include "services/interfaces/workflow/fs2024/prepare/fs2024_tiff_ifd.hpp"
#include "services/interfaces/workflow/fs2024/prepare/fs2024_tiff_predictor.hpp"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <fstream>
#include <stdexcept>

namespace sdl3cpp::tools::fs2024 {

DemTile::DemTile(const std::string& path) : path_(path) {
    const TiffDirectory dir = ReadTiffDirectory(path);
    if (dir.compression != 8 || dir.predictor != 3 ||
        dir.sampleFormat != 3 || dir.bitsPerSample != 32) {
        throw std::runtime_error(
            "DemTile '" + path +
            "': expected DEFLATE + float predictor + float32 samples "
            "(Copernicus GLO-30's own layout)");
    }
    width_ = dir.width;
    height_ = dir.height;
    tileWidth_ = dir.tileWidth;
    tileLength_ = dir.tileLength;
    tileOffsets_ = dir.tileOffsets;
    tileByteCounts_ = dir.tileByteCounts;
    dlon_ = dir.pixelScaleX;
    dlat_ = dir.pixelScaleY;
    lon0_ = dir.tiepointLon;
    lat0_ = dir.tiepointLat;
}

const std::vector<std::uint8_t>& DemTile::DecodedTile(
    std::uint32_t tileIndex) const {
    const auto found = cache_.find(tileIndex);
    if (found != cache_.end()) return found->second;

    std::ifstream in(path_, std::ios::binary);
    in.seekg(tileOffsets_.at(tileIndex));
    std::vector<std::uint8_t> compressed(tileByteCounts_.at(tileIndex));
    in.read(reinterpret_cast<char*>(compressed.data()),
           static_cast<std::streamsize>(compressed.size()));
    if (!in) throw std::runtime_error("DemTile '" + path_ + "': short read");

    const std::size_t decodedSize =
        static_cast<std::size_t>(tileWidth_) * tileLength_ * 4;
    auto decoded =
        InflateZlib(compressed.data(), compressed.size(), decodedSize);
    for (std::uint32_t row = 0; row < tileLength_; ++row) {
        UndoFloatPredictorRow(decoded.data() + row * tileWidth_ * 4,
                             tileWidth_, 4);
    }
    return cache_.emplace(tileIndex, std::move(decoded)).first->second;
}

float DemTile::PixelAt(std::uint32_t row, std::uint32_t col) const {
    const std::uint32_t tilesAcross =
        (width_ + tileWidth_ - 1) / tileWidth_;
    const std::uint32_t tileIndex =
        (row / tileLength_) * tilesAcross + (col / tileWidth_);
    const std::vector<std::uint8_t>& bytes = DecodedTile(tileIndex);
    const std::uint32_t localRow = row % tileLength_;
    const std::uint32_t localCol = col % tileWidth_;
    float value;
    std::memcpy(&value,
               bytes.data() + (localRow * tileWidth_ + localCol) * 4, 4);
    return value;
}

float DemTile::Sample(double lon, double lat) const {
    const double col = (lon - lon0_) / dlon_;
    const double row = (lat0_ - lat) / dlat_;
    if (col < 0.0 || row < 0.0 || col > width_ - 1 || row > height_ - 1) {
        throw std::runtime_error("DemTile::Sample: point outside the tile");
    }
    const auto c0 = static_cast<std::uint32_t>(
        std::min(col, static_cast<double>(width_ - 2)));
    const auto r0 = static_cast<std::uint32_t>(
        std::min(row, static_cast<double>(height_ - 2)));
    const float fc = static_cast<float>(col - c0);
    const float fr = static_cast<float>(row - r0);

    const float p00 = PixelAt(r0, c0);
    const float p10 = PixelAt(r0, c0 + 1);
    const float p01 = PixelAt(r0 + 1, c0);
    const float p11 = PixelAt(r0 + 1, c0 + 1);
    const float top = p00 + (p10 - p00) * fc;
    const float bottom = p01 + (p11 - p01) * fc;
    return top + (bottom - top) * fr;
}

}  // namespace sdl3cpp::tools::fs2024
