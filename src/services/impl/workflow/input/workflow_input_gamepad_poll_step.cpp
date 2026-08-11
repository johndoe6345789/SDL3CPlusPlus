#include "services/interfaces/workflow/input/workflow_input_gamepad_poll_step.hpp"

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

    // Read axes (normalized to [-1, 1])
    auto readAxis = [&](int axis) -> float {
        int16_t raw = SDL_GetJoystickAxis(joystick, axis);
        return raw / 32768.0f;
    };

    context.Set<float>("input.gamepad.left_stick_x", readAxis(SDL_GAMEPAD_AXIS_LEFTX));
    context.Set<float>("input.gamepad.left_stick_y", readAxis(SDL_GAMEPAD_AXIS_LEFTY));
    context.Set<float>("input.gamepad.right_stick_x", readAxis(SDL_GAMEPAD_AXIS_RIGHTX));
    context.Set<float>("input.gamepad.right_stick_y", readAxis(SDL_GAMEPAD_AXIS_RIGHTY));
    context.Set<float>("input.gamepad.trigger_left", readAxis(SDL_GAMEPAD_AXIS_LEFT_TRIGGER));
    context.Set<float>("input.gamepad.trigger_right", readAxis(SDL_GAMEPAD_AXIS_RIGHT_TRIGGER));

    // Read buttons
    auto readButton = [&](int btn) -> bool {
        return SDL_GetJoystickButton(joystick, btn) != 0;
    };

    context.Set<bool>("input.gamepad.button_south", readButton(SDL_GAMEPAD_BUTTON_SOUTH));
    context.Set<bool>("input.gamepad.button_east", readButton(SDL_GAMEPAD_BUTTON_EAST));
    context.Set<bool>("input.gamepad.button_west", readButton(SDL_GAMEPAD_BUTTON_WEST));
    context.Set<bool>("input.gamepad.button_north", readButton(SDL_GAMEPAD_BUTTON_NORTH));
    context.Set<bool>("input.gamepad.button_left_shoulder", readButton(SDL_GAMEPAD_BUTTON_LEFT_SHOULDER));
    context.Set<bool>("input.gamepad.button_right_shoulder", readButton(SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER));
    context.Set<bool>("input.gamepad.button_back", readButton(SDL_GAMEPAD_BUTTON_BACK));
    context.Set<bool>("input.gamepad.button_start", readButton(SDL_GAMEPAD_BUTTON_START));

    if (logger_) {
        logger_->Debug("input.gamepad.poll: Axes and buttons read");
    }
}

}  // namespace sdl3cpp::services::impl
