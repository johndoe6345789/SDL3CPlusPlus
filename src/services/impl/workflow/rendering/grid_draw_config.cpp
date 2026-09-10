#include "services/interfaces/workflow/rendering/grid_draw_config.hpp"

namespace sdl3cpp::services::impl {

GridDrawConfig ReadGridDrawConfig(const nlohmann::json& cfg) {
    GridDrawConfig out;
    out.gridWidth  = cfg.value("grid_width", out.gridWidth);
    out.gridHeight = cfg.value("grid_height", out.gridHeight);
    out.spacing    = cfg.value("grid_spacing", out.spacing);
    out.startX     = cfg.value("grid_start_x", out.startX);
    out.startY     = cfg.value("grid_start_y", out.startY);
    out.rotOffsetX = cfg.value("rotation_offset_x", out.rotOffsetX);
    out.rotOffsetY = cfg.value("rotation_offset_y", out.rotOffsetY);
    out.bgR        = cfg.value("background_color_r", out.bgR);
    out.bgG        = cfg.value("background_color_g", out.bgG);
    out.bgB        = cfg.value("background_color_b", out.bgB);
    out.numFrames  = cfg.value("num_frames", out.numFrames);
    return out;
}

}  // namespace sdl3cpp::services::impl
