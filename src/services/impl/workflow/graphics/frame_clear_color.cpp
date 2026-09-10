#include "services/interfaces/workflow/graphics/frame_clear_color.hpp"

#include <chrono>
#include <stdexcept>

namespace sdl3cpp::services::impl {

FrameClearColor ParseFrameClearColorOrThrow(
    const nlohmann::json* clearColorJson) {
    if (!clearColorJson || !clearColorJson->is_array() ||
        clearColorJson->size() != 4) {
        throw std::runtime_error(
            "graphics.frame.begin requires "
            "clear_color input (array of 4 floats [r,g,b,a])");
    }
    return FrameClearColor{
        (*clearColorJson)[0].get<float>(), (*clearColorJson)[1].get<float>(),
        (*clearColorJson)[2].get<float>(), (*clearColorJson)[3].get<float>()};
}

nlohmann::json BuildFrameBeginOutput(uint32_t frameId, bool skipped,
                                     const nlohmann::json& clearColorJson) {
    if (skipped) {
        return nlohmann::json{{"frame_id", frameId}, {"skipped", true}};
    }
    return nlohmann::json{
        {"frame_id", frameId},
        {"clear_color", clearColorJson},
        {"skipped", false},
        {"timestamp",
         static_cast<double>(std::chrono::high_resolution_clock::now()
                                 .time_since_epoch()
                                 .count())}};
}

}  // namespace sdl3cpp::services::impl
