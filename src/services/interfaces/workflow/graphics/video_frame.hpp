#pragma once

#include <cstdint>
#include <memory>
#include <string>

namespace sdl3cpp::services::impl {

/// The settings a recording file is opened with.
struct VideoEncoderSettings {
    std::string path;  ///< .mp4 or .mkv: the extension picks it
    int width   = 0;   ///< encoded size, rounded down to even
    int height  = 0;
    int fps     = 30;
    int quality = 23;  ///< x264 CRF: lower is sharper and larger
    /// An AAC track at this rate and channel count; 0, no audio.
    int audioRate     = 0;
    int audioChannels = 0;
};

/// One frame of packed 8-bit pixels, rows top to bottom.
struct VideoFrame {
    std::unique_ptr<std::uint8_t[]> pixels;
    int width        = 0;
    int height       = 0;
    bool bgra        = true;  ///< false for RGBA byte order
    std::int64_t pts = 0;     ///< frames since the recording began
};

}  // namespace sdl3cpp::services::impl
