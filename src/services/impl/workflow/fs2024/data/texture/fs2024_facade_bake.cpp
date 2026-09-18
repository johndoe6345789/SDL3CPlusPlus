#include "services/interfaces/workflow/fs2024/data/texture/fs2024_facade_bake.hpp"

#include "services/interfaces/workflow/fs2024/data/texture/fs2024_facade_pixels.hpp"

#include <algorithm>
#include <cmath>

namespace sdl3cpp::fs2024 {

DdsImage BakeFacadeBay(const PggAsset& wall, const DdsImage& wallImage,
                       const PggAsset& window, const DdsImage& windowImage,
                       float bayMetres, float storeyMetres,
                       int pixelsPerMetre) {
    DdsImage facade;
    facade.width = static_cast<int>(std::lround(bayMetres * pixelsPerMetre));
    facade.height =
        static_cast<int>(std::lround(storeyMetres * pixelsPerMetre));
    facade.rgba.assign(
        static_cast<std::size_t>(facade.width) * facade.height * 4, 255);

    for (int y = 0; y < facade.height; ++y) {
        for (int x = 0; x < facade.width; ++x) {
            const float u = static_cast<float>(x) / pixelsPerMetre /
                            std::max(wall.widthMetres, 0.01f);
            const float v = static_cast<float>(y) / pixelsPerMetre /
                            std::max(wall.heightMetres, 0.01f);
            const int sx = static_cast<int>(u * wallImage.width) %
                           std::max(wallImage.width, 1);
            const int sy = static_cast<int>(v * wallImage.height) %
                           std::max(wallImage.height, 1);
            std::copy_n(Texel(wallImage, sx, sy), 4,
                       facade.rgba.data() +
                           (static_cast<std::size_t>(y) * facade.width + x) *
                               4);
        }
    }

    // The window's own metres and offset, measured from the bay's
    // centre and its floor -- v grows downwards in the baked image.
    const int windowWide =
        static_cast<int>(std::lround(window.widthMetres * pixelsPerMetre));
    const int windowHigh =
        static_cast<int>(std::lround(window.heightMetres * pixelsPerMetre));
    const int left = (facade.width - windowWide) / 2 +
                     static_cast<int>(window.offsetX * pixelsPerMetre);
    const int bottom = facade.height -
                       static_cast<int>(window.offsetY * pixelsPerMetre);
    for (int y = 0; y < windowHigh; ++y) {
        const int dy = bottom - windowHigh + y;
        if (dy < 0 || dy >= facade.height) continue;
        for (int x = 0; x < windowWide; ++x) {
            const int dx = left + x;
            if (dx < 0 || dx >= facade.width) continue;
            BlendTexel(facade.rgba.data() +
                     (static_cast<std::size_t>(dy) * facade.width + dx) * 4,
                     Texel(windowImage, x * windowImage.width / windowWide,
                       y * windowImage.height / windowHigh));
        }
    }
    return facade;
}

}  // namespace sdl3cpp::fs2024
