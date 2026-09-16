#pragma once

#include "services/interfaces/workflow/graphics/video_frame.hpp"

#include <array>
#include <cstdint>
#include <string>

namespace video_test {

using sdl3cpp::services::impl::VideoFrame;

/// Grainy stripes scrolling sideways: enough like a game that x264
/// codes B-frames, which is where a frame without a duration was lost.
VideoFrame Scrolling(int width, int height, std::int64_t pts);

/// Packets, encoded width, and where the last packet shown ends.
using Summary = std::array<long long, 3>;

/// Reads @p path back with libavformat; lengths in 30 fps frames.
Summary ReadBack(const std::string& path);

}  // namespace video_test
