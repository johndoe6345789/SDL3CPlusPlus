#pragma once

#include "services/interfaces/workflow/rendering/draw_textured_params.hpp"
#include "services/interfaces/workflow/rendering/draw_textured_transform.hpp"

namespace sdl3cpp::services::impl {

/// Applies `pos_*`/`rot_*`/`scale` in that order; the normal is always +Y.
/// Used when draw.textured has no `facing` parameter.
DrawTexturedTransform BuildFreeTransform(const DrawTexturedParams& params);

}  // namespace sdl3cpp::services::impl
