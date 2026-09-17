#include "services/interfaces/workflow/fs2024/prepare/fs2024_ground_image.hpp"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

#include <algorithm>
#include <cmath>

namespace sdl3cpp::tools::fs2024 {

GroundImage::GroundImage(int width, int height, float originX,
                        float originZ, float metresPerPixel)
    : width_(width), height_(height), originX_(originX), originZ_(originZ),
      metresPerPixel_(metresPerPixel),
      pixels_(static_cast<std::size_t>(width) * height * 3, 0) {}

void GroundImage::SetPixel(int x, int y, std::array<std::uint8_t, 3> rgb) {
    if (x < 0 || y < 0 || x >= width_ || y >= height_) return;
    const std::size_t at = (static_cast<std::size_t>(y) * width_ + x) * 3;
    pixels_[at] = rgb[0];
    pixels_[at + 1] = rgb[1];
    pixels_[at + 2] = rgb[2];
}

std::array<std::uint8_t, 3> GroundImage::Pixel(int x, int y) const {
    x = std::clamp(x, 0, width_ - 1);
    y = std::clamp(y, 0, height_ - 1);
    const std::size_t at = (static_cast<std::size_t>(y) * width_ + x) * 3;
    return {pixels_[at], pixels_[at + 1], pixels_[at + 2]};
}

void GroundImage::FillPolygon(
    const std::vector<std::pair<float, float>>& polygon,
    std::array<std::uint8_t, 3> rgb) {
    if (polygon.size() < 3) return;
    std::vector<std::pair<float, float>> px;
    px.reserve(polygon.size());
    float minY = 1e30f, maxY = -1e30f;
    for (const auto& [x, z] : polygon) {
        const float py = (z - originZ_) / metresPerPixel_;
        px.emplace_back((x - originX_) / metresPerPixel_, py);
        minY = std::min(minY, py);
        maxY = std::max(maxY, py);
    }
    const int rowStart = std::max(0, static_cast<int>(std::floor(minY)));
    const int rowEnd =
        std::min(height_ - 1, static_cast<int>(std::ceil(maxY)));

    for (int row = rowStart; row <= rowEnd; ++row) {
        const float scanY = row + 0.5f;
        std::vector<float> crossings;
        for (std::size_t i = 0; i < px.size(); ++i) {
            const auto& [x0, y0] = px[i];
            const auto& [x1, y1] = px[(i + 1) % px.size()];
            if ((y0 <= scanY && y1 > scanY) || (y1 <= scanY && y0 > scanY)) {
                crossings.push_back(x0 + (scanY - y0) / (y1 - y0) * (x1 - x0));
            }
        }
        std::sort(crossings.begin(), crossings.end());
        for (std::size_t i = 0; i + 1 < crossings.size(); i += 2) {
            const int from = static_cast<int>(std::round(crossings[i]));
            const int to = static_cast<int>(std::round(crossings[i + 1]));
            for (int col = from; col < to; ++col) SetPixel(col, row, rgb);
        }
    }
}

void GroundImage::WritePng(const std::string& path) const {
    stbi_write_png(path.c_str(), width_, height_, 3, pixels_.data(),
                   width_ * 3);
}

}  // namespace sdl3cpp::tools::fs2024
