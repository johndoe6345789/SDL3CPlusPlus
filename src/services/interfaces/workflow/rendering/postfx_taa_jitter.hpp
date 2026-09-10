#pragma once

#include "services/interfaces/workflow_context.hpp"

#include <cstdint>

namespace sdl3cpp::services::impl {

/// Halton sequence for sub-pixel jitter (low discrepancy, covers a
/// pixel well).
float Halton(int index, int base);

/**
 * @brief Jitters `render.proj_matrix` in context for the *next* frame.
 *
 * The current frame was rendered with the jitter applied last frame, so
 * this must run after the frame is drawn but before the next frame's
 * projection matrix is consumed.
 */
void ApplyTaaProjectionJitter(WorkflowContext& context, int frameIdx,
                              uint32_t width, uint32_t height);

}  // namespace sdl3cpp::services::impl
