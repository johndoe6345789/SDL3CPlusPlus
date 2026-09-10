#pragma once

#include "services/interfaces/workflow_context.hpp"

namespace sdl3cpp::services::impl {

/// Accumulated per-frame input, collected by DrainInputEvents from the
/// SDL event queue.
struct PolledInputEvents {
    float mouseRelX = 0.0f, mouseRelY = 0.0f;
    bool keyEscapePressed = false;
    bool keyEnterPressed  = false;
    bool keyUpPressed     = false;
    bool keyDownPressed   = false;
    bool keyQPressed      = false;
    bool mouseLeftPressed = false;
};

/**
 * @brief Drains the SDL event queue for this frame.
 *
 * A quit/close event sets game_running, outer_running and
 * q3.quit_requested directly in context (not through the returned
 * struct, matching the original step). Other events accumulate into the
 * returned PolledInputEvents.
 */
PolledInputEvents DrainInputEvents(WorkflowContext& context);

/// Writes `events` into its input_* context keys.
void WriteInputEventFlags(WorkflowContext& context,
                          const PolledInputEvents& events);

}  // namespace sdl3cpp::services::impl
