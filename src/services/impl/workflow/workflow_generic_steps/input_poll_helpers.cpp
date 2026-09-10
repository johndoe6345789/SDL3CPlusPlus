#include "services/interfaces/workflow/workflow_generic_steps/input_poll_helpers.hpp"

#include <SDL3/SDL.h>

namespace sdl3cpp::services::impl {

PolledInputEvents DrainInputEvents(WorkflowContext& context) {
    PolledInputEvents out;

    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        switch (event.type) {
            case SDL_EVENT_QUIT:
            case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
                context.Set<bool>("game_running", false);
                context.Set<bool>("outer_running", false);
                context.Set<bool>("q3.quit_requested", true);
                break;
            case SDL_EVENT_KEY_DOWN:
                if (event.key.key == SDLK_ESCAPE) {
                    out.keyEscapePressed = true;
                } else if (event.key.key == SDLK_RETURN) {
                    out.keyEnterPressed = true;
                } else if (event.key.key == SDLK_UP) {
                    out.keyUpPressed = true;
                } else if (event.key.key == SDLK_DOWN) {
                    out.keyDownPressed = true;
                } else if (event.key.key == SDLK_Q) {
                    out.keyQPressed = true;
                }
                break;
            case SDL_EVENT_MOUSE_BUTTON_DOWN:
                if (event.button.button == SDL_BUTTON_LEFT) {
                    out.mouseLeftPressed = true;
                }
                break;
            case SDL_EVENT_MOUSE_MOTION:
                out.mouseRelX += event.motion.xrel;
                out.mouseRelY += event.motion.yrel;
                break;
        }
    }
    return out;
}

void WriteInputEventFlags(WorkflowContext& context,
                          const PolledInputEvents& events) {
    context.Set<float>("input_mouse_rel_x", events.mouseRelX);
    context.Set<float>("input_mouse_rel_y", events.mouseRelY);
    context.Set<bool>("input_key_escape_pressed", events.keyEscapePressed);
    context.Set<bool>("input_key_enter_pressed", events.keyEnterPressed);
    context.Set<bool>("input_key_up_pressed", events.keyUpPressed);
    context.Set<bool>("input_key_down_pressed", events.keyDownPressed);
    context.Set<bool>("input_key_q_pressed", events.keyQPressed);
    context.Set<bool>("input_mouse_left_pressed", events.mouseLeftPressed);
}

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
