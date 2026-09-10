#pragma once

namespace sdl3cpp::services::impl {

/**
 * @brief Hand-off between the atomic steps that replaced `postfx.composite`.
 *
 * `postfx.composite_draw` publishes the outcome of the composite pass under
 * kPostfxCompositeStateKey; the steps that follow it in the workflow use that
 * outcome to decide whether they apply to this frame.  Splitting the original
 * monolithic step this way keeps each step independently skippable, which the
 * flat (branch-free) workflow executor requires.
 */
inline constexpr const char* kPostfxCompositeStateKey =
    "postfx_composite_state";

/// Required GPU resources were absent — nothing was drawn this frame.
inline constexpr const char* kPostfxCompositeStateMissing = "missing";

/// Resources were present but the render pass could not be started.
inline constexpr const char* kPostfxCompositeStateFailed = "failed";

/// The composite pass ran; the swapchain holds the composited image.
inline constexpr const char* kPostfxCompositeStateDrawn = "drawn";

}  // namespace sdl3cpp::services::impl
