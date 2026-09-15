#pragma once

namespace sdl3cpp::q3 {

/// Virtual coordinate space every overlay draw call works in, as ioq3
/// does. Only the rasterisation resolution changes with the window.
inline constexpr int kVirtualWidth = 640;
inline constexpr int kVirtualHeight = 480;

/// Upper bound on the rasterised overlay, to keep the software renderer
/// affordable on very large or HiDPI displays.
inline constexpr int kMaxOverlayScale = 4;

struct OverlaySize {
    int width;
    int height;
    int scale;
};

/**
 * @brief Pick the pixel size to rasterise the overlay at.
 *
 * ioq3 draws its UI in 640x480 coordinates but rasterises at the native
 * resolution, so text stays sharp. Rendering into a literal 640x480
 * surface and stretching the result instead resamples every glyph.
 *
 * An integer multiple of the virtual size is chosen so glyphs land on
 * whole pixels; the result is clamped to kMaxOverlayScale and never
 * drops below 1x, including for degenerate window sizes.
 */
OverlaySize ChooseOverlaySize(int targetWidth, int targetHeight);

}  // namespace sdl3cpp::q3
