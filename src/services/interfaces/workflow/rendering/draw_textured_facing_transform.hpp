#pragma once

#include "services/interfaces/workflow/rendering/draw_textured_transform.hpp"

#include <string>

namespace sdl3cpp::services::impl {

/// Orients a plane to face one of the 6 cardinal directions
/// ("up"/"down"/"north"/"south"/"east"/"west") at `pos`; `rot_*` is ignored
/// in this mode, matching the original. Scale (if any) is applied by the
/// caller after this returns. An unrecognized `facing` leaves the model at
/// identity (untranslated) with a +Y normal.
DrawTexturedTransform BuildFacingTransform(const std::string& facing,
                                           const glm::vec3& pos);

}  // namespace sdl3cpp::services::impl
