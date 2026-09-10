#pragma once

#include "services/interfaces/workflow/graphics/frame_clear_color.hpp"
#include "services/interfaces/workflow/graphics/frame_swapchain.hpp"

#include <string>

namespace sdl3cpp::services::impl {

/// One-line trace description of a started frame, for graphics.frame.begin's
/// logger.
std::string DescribeFrameBegin(const FrameClearColor& cc,
                               const SwapchainAcquireResult& swap);

}  // namespace sdl3cpp::services::impl
