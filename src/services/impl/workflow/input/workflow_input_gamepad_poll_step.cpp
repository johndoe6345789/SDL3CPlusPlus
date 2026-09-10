#include "services/interfaces/workflow/input/workflow_input_gamepad_poll_step.hpp"
#include "services/interfaces/workflow/input/gamepad_state_read.hpp"

#include <SDL3/SDL.h>

#include <stdexcept>
#include <string>

namespace sdl3cpp::services::impl {

WorkflowInputGamepadPollStep::WorkflowInputGamepadPollStep(
    std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)) {}

std::string WorkflowInputGamepadPollStep::GetPluginId() const {
    return "input.gamepad.poll";
}

void WorkflowInputGamepadPollStep::Execute(
    const WorkflowStepDefinition& step, WorkflowContext& context) {

    if (logger_) {
        logger_->Trace("WorkflowInputGamepadPollStep", "Execute", "Entry");
    }

    // Discover first available joystick
    int numJoysticks = 0;
    SDL_JoystickID* joystickIds = SDL_GetJoysticks(&numJoysticks);
    SDL_Joystick* joystick = nullptr;

    if (joystickIds && numJoysticks > 0) {
        joystick = SDL_OpenJoystick(joystickIds[0]);
        if (logger_ && joystick) {
            logger_->Debug("input.gamepad.poll: Gamepad found, ID=" +
                           std::to_string(joystickIds[0]));
        }
    }
    if (joystickIds) {
        SDL_free(joystickIds);
    }

    if (!joystick) {
        context.Set<bool>("input.gamepad.connected", false);
        if (logger_) {
            logger_->Debug("input.gamepad.poll: No gamepad connected");
        }
        return;
    }

    context.Set<bool>("input.gamepad.connected", true);

    const GamepadPollState state = ReadGamepadState(joystick);

    context.Set<float>("input.gamepad.left_stick_x", state.left_stick_x);
    context.Set<float>("input.gamepad.left_stick_y", state.left_stick_y);
    context.Set<float>("input.gamepad.right_stick_x", state.right_stick_x);
    context.Set<float>("input.gamepad.right_stick_y", state.right_stick_y);
    context.Set<float>("input.gamepad.trigger_left", state.trigger_left);
    context.Set<float>("input.gamepad.trigger_right", state.trigger_right);

    context.Set<bool>("input.gamepad.button_south", state.button_south);
    context.Set<bool>("input.gamepad.button_east", state.button_east);
    context.Set<bool>("input.gamepad.button_west", state.button_west);
    context.Set<bool>("input.gamepad.button_north", state.button_north);
    context.Set<bool>("input.gamepad.button_left_shoulder",
                      state.button_left_shoulder);
    context.Set<bool>("input.gamepad.button_right_shoulder",
                      state.button_right_shoulder);
    context.Set<bool>("input.gamepad.button_back", state.button_back);
    context.Set<bool>("input.gamepad.button_start", state.button_start);

    if (logger_) {
        logger_->Debug("input.gamepad.poll: Axes and buttons read");
    }
}

}  // namespace sdl3cpp::services::impl
