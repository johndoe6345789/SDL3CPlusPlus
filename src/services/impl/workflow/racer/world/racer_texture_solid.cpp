#include "services/interfaces/workflow/racer/world/racer_gpu_upload.hpp"

namespace sdl3cpp::services::impl {
namespace {

constexpr std::size_t kStrayAlphaShare = 16;  // 1 in 16: about 6%

}  // namespace

bool SolidifyRacerTexture(RacerTexture& texture) {
    // The N64 art's one-bit alpha leaves stray clear texels in opaque
    // rock and metal, which the cut-out test turns into pinholes of sky.
    // A texture that is nearly all solid is drawn wholly solid.
    std::size_t clear = 0;
    const std::size_t texels = texture.rgba.size() / 4;
    for (std::size_t i = 0; i < texels; ++i) {
        clear += texture.rgba[4 * i + 3] < 128 ? 1 : 0;
    }
    if (clear == 0 || clear * kStrayAlphaShare > texels) return false;
    for (std::size_t i = 0; i < texels; ++i) texture.rgba[4 * i + 3] = 255;
    return true;
}

}  // namespace sdl3cpp::services::impl
