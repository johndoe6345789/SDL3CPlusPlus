#include "services/interfaces/workflow/gta5/player/gta5_gamepad.hpp"

#include <algorithm>
#include <cmath>

namespace sdl3cpp::services::impl {
namespace {

constexpr float kDeadzone = 0.2f;

float Axis(SDL_Gamepad* gamepad, SDL_GamepadAxis axis) {
    return static_cast<float>(SDL_GetGamepadAxis(gamepad, axis)) / 32767.f;
}

/// A stick past its round deadzone, rescaled so the edge still reads 1.
glm::vec2 Stick(SDL_Gamepad* gamepad, SDL_GamepadAxis x, SDL_GamepadAxis y) {
    const glm::vec2 raw(Axis(gamepad, x), Axis(gamepad, y));
    const float lean = std::min(glm::length(raw), 1.f);
    if (lean < kDeadzone) return glm::vec2(0.f);
    return raw / glm::length(raw) * ((lean - kDeadzone) / (1.f - kDeadzone));
}

}  // namespace

bool ReadGta5Pad(SDL_Gamepad*& gamepad, Gta5Pad& pad) {
    if (gamepad && !SDL_GamepadConnected(gamepad)) {
        SDL_CloseGamepad(gamepad);
        gamepad = nullptr;
    }
    if (!gamepad) {
        int count = 0;
        SDL_JoystickID* ids = SDL_GetGamepads(&count);
        if (ids && count > 0) gamepad = SDL_OpenGamepad(ids[0]);
        SDL_free(ids);
        if (!gamepad) return false;
    }
    pad.move = Stick(gamepad, SDL_GAMEPAD_AXIS_LEFTX, SDL_GAMEPAD_AXIS_LEFTY);
    pad.look = Stick(gamepad, SDL_GAMEPAD_AXIS_RIGHTX, SDL_GAMEPAD_AXIS_RIGHTY);
    pad.brake = std::clamp(Axis(gamepad, SDL_GAMEPAD_AXIS_LEFT_TRIGGER), 0.f,
                           1.f);
    pad.throttle = std::clamp(
        Axis(gamepad, SDL_GAMEPAD_AXIS_RIGHT_TRIGGER), 0.f, 1.f);
    for (int b = 0; b < SDL_GAMEPAD_BUTTON_COUNT; ++b) {
        pad.held[b] =
            SDL_GetGamepadButton(gamepad, static_cast<SDL_GamepadButton>(b));
    }
    return true;
}

}  // namespace sdl3cpp::services::impl
