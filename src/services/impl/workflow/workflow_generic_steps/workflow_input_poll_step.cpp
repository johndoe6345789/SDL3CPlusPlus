#include "services/interfaces/workflow/workflow_generic_steps/workflow_input_poll_step.hpp"
#include "services/interfaces/workflow_step_definition.hpp"
#include "services/interfaces/workflow_context.hpp"

#include <SDL3/SDL.h>
#include <nlohmann/json.hpp>
#include <utility>

namespace sdl3cpp::services::impl {

WorkflowInputPollStep::WorkflowInputPollStep(std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)) {}

std::string WorkflowInputPollStep::GetPluginId() const {
    return "input.poll";
}

void WorkflowInputPollStep::Execute(
    const WorkflowStepDefinition& step, WorkflowContext& context) {

    float mouseRelX = 0.0f, mouseRelY = 0.0f;
    bool keyEscapePressed = false;
    bool keyEnterPressed = false;
    bool keyUpPressed = false;
    bool keyDownPressed = false;
    bool keyQPressed = false;
    bool mouseLeftPressed = false;

    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        switch (event.type) {
            case SDL_EVENT_QUIT:
            case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
                context.Set<bool>("game_running",    false);
                context.Set<bool>("outer_running",   false);
                context.Set<bool>("q3.quit_requested", true);
                break;
            case SDL_EVENT_KEY_DOWN:
                if (event.key.key == SDLK_ESCAPE) {
                    keyEscapePressed = true;
                } else if (event.key.key == SDLK_RETURN) {
                    keyEnterPressed = true;
                } else if (event.key.key == SDLK_UP) {
                    keyUpPressed = true;
                } else if (event.key.key == SDLK_DOWN) {
                    keyDownPressed = true;
                } else if (event.key.key == SDLK_Q) {
                    keyQPressed = true;
                }
                break;
            case SDL_EVENT_MOUSE_BUTTON_DOWN:
                if (event.button.button == SDL_BUTTON_LEFT) mouseLeftPressed = true;
                break;
            case SDL_EVENT_MOUSE_MOTION:
                mouseRelX += event.motion.xrel;
                mouseRelY += event.motion.yrel;
                break;
        }
    }

    // Store accumulated mouse motion for this frame
    context.Set<float>("input_mouse_rel_x", mouseRelX);
    context.Set<float>("input_mouse_rel_y", mouseRelY);
    context.Set<bool>("input_key_escape_pressed", keyEscapePressed);
    context.Set<bool>("input_key_enter_pressed", keyEnterPressed);
    context.Set<bool>("input_key_up_pressed", keyUpPressed);
    context.Set<bool>("input_key_down_pressed", keyDownPressed);
    context.Set<bool>("input_key_q_pressed", keyQPressed);
    context.Set<bool>("input_mouse_left_pressed", mouseLeftPressed);

    // Read keyboard state (snapshot, not event-based)
    const bool* keyState = SDL_GetKeyboardState(nullptr);
    if (keyState) {
        context.Set<bool>("input_key_w", keyState[SDL_SCANCODE_W]);
        context.Set<bool>("input_key_a", keyState[SDL_SCANCODE_A]);
        context.Set<bool>("input_key_s", keyState[SDL_SCANCODE_S]);
        context.Set<bool>("input_key_d", keyState[SDL_SCANCODE_D]);
        context.Set<bool>("input_key_space", keyState[SDL_SCANCODE_SPACE]);
        context.Set<bool>("input_key_shift", keyState[SDL_SCANCODE_LSHIFT]);
        context.Set<bool>("input_key_ctrl", keyState[SDL_SCANCODE_LCTRL]);
        context.Set<bool>("input_mouse_left", (SDL_GetMouseState(nullptr, nullptr) & SDL_BUTTON_LMASK) != 0);
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
}

}  // namespace sdl3cpp::services::impl
