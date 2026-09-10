#include "services/interfaces/workflow/graphics/frame_begin_describe.hpp"

namespace sdl3cpp::services::impl {

std::string DescribeFrameBegin(const FrameClearColor& cc,
                               const SwapchainAcquireResult& swap) {
    return "clear_color=(" + std::to_string(cc.r) + "," + std::to_string(cc.g) +
           "," + std::to_string(cc.b) + "," + std::to_string(cc.a) +
           "), swapchain=" + std::to_string(swap.width) + "x" +
           std::to_string(swap.height);
}

}  // namespace sdl3cpp::services::impl
