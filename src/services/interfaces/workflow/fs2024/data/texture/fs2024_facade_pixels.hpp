#pragma once

#include "services/interfaces/workflow/fs2024/data/texture/fs2024_dds_texture.hpp"

#include <algorithm>

namespace sdl3cpp::fs2024 {

/// Clamped point sample. These textures are composited at close to
/// their own resolution, and a facade's brick courses stay crisper
/// without filtering.
inline const std::uint8_t* Texel(const DdsImage& image, int x, int y) {
    x = std::clamp(x, 0, image.width - 1);
    y = std::clamp(y, 0, image.height - 1);
    return image.rgba.data() +
           (static_cast<std::size_t>(y) * image.width + x) * 4;
}

/// Source-over blend of an RGBA texel onto an opaque one.
inline void BlendTexel(std::uint8_t* dst, const std::uint8_t* src) {
    const int alpha = src[3];
    for (int c = 0; c < 3; ++c) {
        dst[c] = static_cast<std::uint8_t>(
            (src[c] * alpha + dst[c] * (255 - alpha)) / 255);
    }
    dst[3] = 255;
}

}  // namespace sdl3cpp::fs2024
