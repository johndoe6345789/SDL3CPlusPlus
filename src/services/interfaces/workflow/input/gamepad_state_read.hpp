#pragma once

#include <SDL3/SDL.h>

namespace sdl3cpp::services::impl {

/// A single poll's worth of gamepad axis/button values, normalized the
/// same way the workflow context stores them.
struct GamepadPollState {
    float left_stick_x  = 0.0f;
    float left_stick_y  = 0.0f;
    float right_stick_x = 0.0f;
    float right_stick_y = 0.0f;
    float trigger_left  = 0.0f;
    float trigger_right = 0.0f;

    bool button_south          = false;
    bool button_east           = false;
    bool button_west           = false;
    bool button_north          = false;
    bool button_left_shoulder  = false;
    bool button_right_shoulder = false;
    bool button_back           = false;
    bool button_start          = false;
};

/**
 * @brief Reads all standard axes and buttons from an already-open
 *        joystick, normalizing axes to [-1.0, 1.0] from SDL's
 *        [-32768, 32767] range.
 */
GamepadPollState ReadGamepadState(SDL_Joystick* joystick);

}  // namespace sdl3cpp::services::impl
