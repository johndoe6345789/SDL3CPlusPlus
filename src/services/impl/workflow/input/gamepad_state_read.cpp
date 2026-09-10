#include "services/interfaces/workflow/input/gamepad_state_read.hpp"

namespace sdl3cpp::services::impl {

namespace {

float ReadAxis(SDL_Joystick* joystick, int axis) {
    const int16_t raw = SDL_GetJoystickAxis(joystick, axis);
    return raw / 32768.0f;
}

bool ReadButton(SDL_Joystick* joystick, int button) {
    return SDL_GetJoystickButton(joystick, button) != 0;
}

}  // namespace

GamepadPollState ReadGamepadState(SDL_Joystick* joystick) {
    GamepadPollState state;

    state.left_stick_x = ReadAxis(joystick, SDL_GAMEPAD_AXIS_LEFTX);
    state.left_stick_y = ReadAxis(joystick, SDL_GAMEPAD_AXIS_LEFTY);
    state.right_stick_x = ReadAxis(joystick, SDL_GAMEPAD_AXIS_RIGHTX);
    state.right_stick_y = ReadAxis(joystick, SDL_GAMEPAD_AXIS_RIGHTY);
    state.trigger_left = ReadAxis(joystick, SDL_GAMEPAD_AXIS_LEFT_TRIGGER);
    state.trigger_right = ReadAxis(joystick, SDL_GAMEPAD_AXIS_RIGHT_TRIGGER);

    state.button_south = ReadButton(joystick, SDL_GAMEPAD_BUTTON_SOUTH);
    state.button_east = ReadButton(joystick, SDL_GAMEPAD_BUTTON_EAST);
    state.button_west = ReadButton(joystick, SDL_GAMEPAD_BUTTON_WEST);
    state.button_north = ReadButton(joystick, SDL_GAMEPAD_BUTTON_NORTH);
    state.button_left_shoulder =
        ReadButton(joystick, SDL_GAMEPAD_BUTTON_LEFT_SHOULDER);
    state.button_right_shoulder =
        ReadButton(joystick, SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER);
    state.button_back = ReadButton(joystick, SDL_GAMEPAD_BUTTON_BACK);
    state.button_start = ReadButton(joystick, SDL_GAMEPAD_BUTTON_START);

    return state;
}

}  // namespace sdl3cpp::services::impl
