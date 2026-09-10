#pragma once

#include <array>

namespace sdl3cpp::services::impl {

/// Six vertices (two triangles) of five floats each: {x, y, z, u, v}.
using GpuOverlayQuad = std::array<float, 6 * 5>;

/**
 * @brief Builds the overlay quad in normalised device coordinates.
 *
 * The quad is pinned to the top-right corner of the viewport with a fixed
 * margin, and its UVs map the whole overlay texture.  Pixel y grows downwards
 * while NDC y grows upwards, so the vertical axis is flipped here.
 *
 * @param viewportWidth   Viewport width in pixels; values <= 0 are clamped
 * to 1.
 * @param viewportHeight  Viewport height in pixels; values <= 0 are clamped
 * to 1.
 * @param overlayWidth    Overlay texture width in pixels.
 * @param overlayHeight   Overlay texture height in pixels.
 * @param margin          Distance from the top and right edges, in pixels.
 */
GpuOverlayQuad BuildGpuOverlayQuad(int viewportWidth, int viewportHeight,
                                   int overlayWidth, int overlayHeight,
                                   float margin = 10.0f);

}  // namespace sdl3cpp::services::impl
