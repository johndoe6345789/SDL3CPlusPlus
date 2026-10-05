#include "services/interfaces/workflow/racer/data/racer_asset_library.hpp"

namespace sdl3cpp::services::impl {

RacerTexture MirrorRacerTexture(const RacerTexture& texture,
                                bool doubleWidth, bool doubleHeight) {
    if ((!doubleWidth && !doubleHeight) || texture.rgba.empty()) {
        return texture;
    }
    RacerTexture out;
    out.width = texture.width * (doubleWidth ? 2 : 1);
    out.height = texture.height * (doubleHeight ? 2 : 1);
    out.rgba.resize(4 * static_cast<std::size_t>(out.width) * out.height);
    for (int y = 0; y < out.height; ++y) {
        const int sy = y < texture.height ? y : out.height - 1 - y;
        for (int x = 0; x < out.width; ++x) {
            const int sx = x < texture.width ? x : out.width - 1 - x;
            const std::size_t src =
                4 * (static_cast<std::size_t>(sy) * texture.width + sx);
            const std::size_t dst =
                4 * (static_cast<std::size_t>(y) * out.width + x);
            for (int c = 0; c < 4; ++c) {
                out.rgba[dst + c] = texture.rgba[src + c];
            }
        }
    }
    return out;
}

}  // namespace sdl3cpp::services::impl
