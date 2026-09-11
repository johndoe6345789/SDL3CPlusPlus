#pragma once

#include "services/interfaces/workflow/gta5/gta5_draw_instances.hpp"

namespace sdl3cpp::services::impl {

/// Bind what every draw of the batch shares -- its instance matrices and
/// the camera uniforms -- for the pipeline just bound. Called again
/// after switching to the blended pipeline.
void BindGta5BatchShared(const Gta5StreamState& state,
                         const Gta5DrawContext& draw);

/// Bind one geometry arena block's vertex and index buffers.
void BindGta5ArenaBlock(const Gta5StreamState& state,
                        const Gta5DrawContext& draw, int block);

}  // namespace sdl3cpp::services::impl
