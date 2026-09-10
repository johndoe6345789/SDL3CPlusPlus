#include "services/interfaces/workflow/workflow_generic_steps/input_keyboard_snapshot.hpp"

#include <SDL3/SDL.h>

namespace sdl3cpp::services::impl {

void WriteKeyboardSnapshot(WorkflowContext& context) {
    const bool* keyState = SDL_GetKeyboardState(nullptr);
    if (!keyState) return;

    context.Set<bool>("input_key_w", keyState[SDL_SCANCODE_W]);
    context.Set<bool>("input_key_a", keyState[SDL_SCANCODE_A]);
    context.Set<bool>("input_key_s", keyState[SDL_SCANCODE_S]);
    context.Set<bool>("input_key_d", keyState[SDL_SCANCODE_D]);
    context.Set<bool>("input_key_space", keyState[SDL_SCANCODE_SPACE]);
    context.Set<bool>("input_key_shift", keyState[SDL_SCANCODE_LSHIFT]);
    context.Set<bool>("input_key_ctrl", keyState[SDL_SCANCODE_LCTRL]);
    context.Set<bool>("input_mouse_left", (SDL_GetMouseState(nullptr, nullptr) &
                                           SDL_BUTTON_LMASK) != 0);
    context.Set<bool>("input_key_1", keyState[SDL_SCANCODE_1]);
    context.Set<bool>("input_key_2", keyState[SDL_SCANCODE_2]);
    context.Set<bool>("input_key_3", keyState[SDL_SCANCODE_3]);
    context.Set<bool>("input_key_4", keyState[SDL_SCANCODE_4]);
    context.Set<bool>("input_key_5", keyState[SDL_SCANCODE_5]);
    context.Set<bool>("input_key_6", keyState[SDL_SCANCODE_6]);
    context.Set<bool>("input_key_7", keyState[SDL_SCANCODE_7]);
    context.Set<bool>("input_key_8", keyState[SDL_SCANCODE_8]);
    context.Set<bool>("input_key_9", keyState[SDL_SCANCODE_9]);
}

}  // namespace sdl3cpp::services::impl
