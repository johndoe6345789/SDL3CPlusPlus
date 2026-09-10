#pragma once

#include <cstdint>
#include <nlohmann/json.hpp>

namespace sdl3cpp::services::impl {

/// The four clear-color channels read from the `clear_color` input.
struct FrameClearColor {
    float r;
    float g;
    float b;
    float a;
};

/**
 * @brief Validates and extracts `clearColorJson` as [r,g,b,a].
 * @throws std::runtime_error if it is not a 4-element array.
 */
FrameClearColor ParseFrameClearColorOrThrow(
    const nlohmann::json* clearColorJson);

/**
 * @brief Builds the `frame_id` output value: `{frame_id, skipped}` when
 * skipped, else `{frame_id, clear_color, skipped, timestamp}`.
 */
nlohmann::json BuildFrameBeginOutput(uint32_t frameId, bool skipped,
                                     const nlohmann::json& clearColorJson);

}  // namespace sdl3cpp::services::impl
