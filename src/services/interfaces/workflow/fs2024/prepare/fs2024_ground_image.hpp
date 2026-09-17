#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <vector>

namespace sdl3cpp::tools::fs2024 {

/// A flat RGB image the ground texture is painted into, in engine (x,
/// z) metres mapped by `originX/originZ/metresPerPixel`. Filled with
/// `base` first; land-cover colouring happens per-pixel by the caller
/// before pavement polygons are drawn solid on top -- the same order
/// python/fs2024/ground_paint.py used, and for the same reason: a
/// road or apron has no fine detail to be softened by noise.
class GroundImage {
public:
    GroundImage(int width, int height, float originX, float originZ,
               float metresPerPixel);

    int Width() const { return width_; }
    int Height() const { return height_; }
    void SetPixel(int x, int y, std::array<std::uint8_t, 3> rgb);
    std::array<std::uint8_t, 3> Pixel(int x, int y) const;

    /// Fills a simple (possibly non-convex) polygon solid, in engine
    /// metres. A plain scanline fill: pavement is flat colour, so it
    /// needs no antialiasing the way fine line markings would.
    void FillPolygon(const std::vector<std::pair<float, float>>& polygon,
                     std::array<std::uint8_t, 3> rgb);

    /// Writes a PNG (lossless -- flat pavement colour survives it
    /// perfectly, unlike the JPEG blur that motivated moving runway
    /// markings to the analytic shader in the first place).
    void WritePng(const std::string& path) const;

private:
    int width_, height_;
    float originX_, originZ_, metresPerPixel_;
    std::vector<std::uint8_t> pixels_;  ///< RGB, row-major
};

}  // namespace sdl3cpp::tools::fs2024
