#pragma once

#include <cstdint>
#include <nlohmann/json.hpp>

namespace sdl3cpp::services::impl {

/// Layout/rendering parameters for one render.grid.draw frame, read
/// from the grid.config populated by render.grid.setup.
struct GridDrawConfig {
    uint32_t gridWidth  = 11u;
    uint32_t gridHeight = 11u;
    float spacing       = 3.0f;
    float startX        = -15.0f;
    float startY        = -15.0f;
    float rotOffsetX    = 0.21f;
    float rotOffsetY    = 0.37f;
    float bgR           = 0.18f;
    float bgG           = 0.18f;
    float bgB           = 0.18f;
    uint32_t numFrames  = 600u;
};

GridDrawConfig ReadGridDrawConfig(const nlohmann::json& cfg);

}  // namespace sdl3cpp::services::impl
