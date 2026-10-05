#include "services/interfaces/workflow/racer/data/racer_upscale.hpp"

#include <cstring>

namespace sdl3cpp::services::impl {
namespace {

std::uint32_t PixelAt(const std::vector<std::uint8_t>& rgba, int width,
                      int x, int y) {
    std::uint32_t value = 0;
    std::memcpy(&value, &rgba[4 * (static_cast<std::size_t>(y) * width + x)],
                4);
    return value;
}

void StorePixel(std::vector<std::uint8_t>& rgba, std::size_t index,
                std::uint32_t value) {
    std::memcpy(&rgba[4 * index], &value, 4);
}

}  // namespace

std::vector<std::uint8_t> UpscaleScale2x(const std::vector<std::uint8_t>& rgba,
                                         int width, int height) {
    const int outWidth = width * 2;
    std::vector<std::uint8_t> out(4 * static_cast<std::size_t>(outWidth) *
                                  height * 2);
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            // Edges repeat the border pixel, so a missing neighbour
            // never matches anything except the pixel itself.
            const std::uint32_t e = PixelAt(rgba, width, x, y);
            const std::uint32_t b = PixelAt(rgba, width, x, y > 0 ? y - 1 : y);
            const std::uint32_t d = PixelAt(rgba, width, x > 0 ? x - 1 : x, y);
            const std::uint32_t f =
                PixelAt(rgba, width, x < width - 1 ? x + 1 : x, y);
            const std::uint32_t h =
                PixelAt(rgba, width, x, y < height - 1 ? y + 1 : y);

            const std::uint32_t e0 = (d == b && b != f && d != h) ? d : e;
            const std::uint32_t e1 = (b == f && b != d && f != h) ? f : e;
            const std::uint32_t e2 = (d == h && d != b && h != f) ? d : e;
            const std::uint32_t e3 = (h == f && d != h && b != f) ? f : e;

            const std::size_t row0 = static_cast<std::size_t>(2 * y) * outWidth;
            const std::size_t row1 = row0 + outWidth;
            const std::size_t col = static_cast<std::size_t>(2 * x);
            StorePixel(out, row0 + col, e0);
            StorePixel(out, row0 + col + 1, e1);
            StorePixel(out, row1 + col, e2);
            StorePixel(out, row1 + col + 1, e3);
        }
    }
    return out;
}

std::vector<std::uint8_t> UpscaleRgba(const std::vector<std::uint8_t>& rgba,
                                      int width, int height, int factor) {
    if (factor < 2 || (factor & (factor - 1)) != 0) return {};
    std::vector<std::uint8_t> current = rgba;
    for (int scale = factor; scale > 1; scale /= 2) {
        current = UpscaleScale2x(current, width, height);
        width *= 2;
        height *= 2;
    }
    return current;
}

}  // namespace sdl3cpp::services::impl
