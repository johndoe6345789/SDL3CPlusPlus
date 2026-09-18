#include "services/interfaces/workflow/fs2024/data/cgl/fs2024_lcg_tile.hpp"

#include <webp/decode.h>

#include <cstring>
#include <stdexcept>

namespace sdl3cpp::fs2024 {

Fs2024LcgImage DecodeFs2024LcgTile(const std::vector<std::uint8_t>& payload) {
    Fs2024LcgImage image;
    std::uint8_t* pixels = WebPDecodeRGBA(payload.data(), payload.size(),
                                          &image.width, &image.height);
    if (!pixels) throw std::runtime_error("lcg tile: not a WebP image");
    image.rgba.assign(pixels, pixels + static_cast<std::size_t>(image.width) *
                                           image.height * 4);
    WebPFree(pixels);
    return image;
}

}  // namespace sdl3cpp::fs2024
