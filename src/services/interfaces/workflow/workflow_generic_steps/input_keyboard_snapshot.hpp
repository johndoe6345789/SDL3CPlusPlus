#pragma once

#include "services/interfaces/workflow_context.hpp"

namespace sdl3cpp::services::impl {

/// Writes a keyboard/mouse-button snapshot (SDL_GetKeyboardState,
/// SDL_GetMouseState) into its input_key_*/input_mouse_left keys. A
/// no-op if SDL has no keyboard state yet.
void WriteKeyboardSnapshot(WorkflowContext& context);

}  // namespace sdl3cpp::services::impl
