#pragma once

#include "services/interfaces/workflow/rendering/draw_textured_params.hpp"

#include <glm/glm.hpp>

namespace sdl3cpp::services::impl {

/// The model matrix and outward surface normal for one draw.textured call.
struct DrawTexturedTransform {
    glm::mat4 model;
    glm::vec3 normal;
};

/**
 * @brief Builds the model matrix and surface normal for `params`.
 *
 * With a non-empty `facing` ("up"/"down"/"north"/"south"/"east"/"west"),
 * orients the plane to face that direction at `pos_*` and derives the
 * normal from it; `rot_*` is ignored in that mode, matching the original.
 * With no `facing`, applies `pos_*`/`rot_*`/`scale` in that order and the
 * normal is always +Y.
 */
DrawTexturedTransform BuildDrawTexturedTransform(
    const DrawTexturedParams& params);

}  // namespace sdl3cpp::services::impl
