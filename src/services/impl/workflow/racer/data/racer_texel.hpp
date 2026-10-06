#pragma once

#include "services/interfaces/workflow/racer/data/racer_texture.hpp"

#include <array>
#include <cstddef>
#include <vector>

namespace sdl3cpp::services::impl {

using RacerRgba = std::array<std::uint8_t, 4>;

/// Texel `index` of a texture's pixel data, through `palette` for the
/// indexed formats; magenta when the data runs out.
RacerRgba RacerTexel(const std::vector<std::uint8_t>& pixels,
                     const std::vector<RacerRgba>& palette,
                     RacerTextureFormat format, std::size_t index);

/// How many texels `bytes` of pixel data hold in `format`.
std::size_t RacerTexelsIn(std::size_t bytes, RacerTextureFormat format);

}  // namespace sdl3cpp::services::impl
