#pragma once

#include <cstdint>
#include <vector>

namespace sdl3cpp::services::impl {

/// One pass of Scale2x (EPX). Each source pixel becomes a 2x2 block; a
/// corner takes a neighbour's colour only when the two neighbours beside
/// it agree and the pixel itself differs from both. This keeps the hard
/// edges of palette art sharp where a bilinear filter would blur them.
std::vector<std::uint8_t> UpscaleScale2x(const std::vector<std::uint8_t>& rgba,
                                         int width, int height);

/// Repeats Scale2x until `factor` is reached. `factor` must be a power
/// of two of at least 2; anything else returns an empty vector.
std::vector<std::uint8_t> UpscaleRgba(const std::vector<std::uint8_t>& rgba,
                                      int width, int height, int factor);

}  // namespace sdl3cpp::services::impl
