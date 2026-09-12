#include "services/interfaces/workflow/gta5/player/gta5_gamepad.hpp"

namespace sdl3cpp::services::impl {
namespace {

constexpr float kWalk = 0.35f;  // stick lean that counts as a key

void Press(nlohmann::json& keys, bool down, const char* name) {
    if (down) keys[name] = true;
}

void Foot(const Gta5Pad& p, nlohmann::json& keys) {
    Press(keys, p.move.y < -kWalk, "W");
    Press(keys, p.move.y > kWalk, "S");
    Press(keys, p.move.x < -kWalk, "A");
    Press(keys, p.move.x > kWalk, "D");
    Press(keys, p.Held(SDL_GAMEPAD_BUTTON_SOUTH), "Left Shift");  // sprint
    Press(keys, p.Held(SDL_GAMEPAD_BUTTON_WEST), "Space");  // jump, rise
    Press(keys, p.Held(SDL_GAMEPAD_BUTTON_EAST), "R");       // reload
    Press(keys, p.Held(SDL_GAMEPAD_BUTTON_LEFT_STICK), "Left Ctrl");
    Press(keys, p.Held(SDL_GAMEPAD_BUTTON_LEFT_SHOULDER), "Q");  // wheel
    Press(keys, p.Held(SDL_GAMEPAD_BUTTON_DPAD_RIGHT), "E");  // shop door
    Press(keys, p.Held(SDL_GAMEPAD_BUTTON_DPAD_LEFT), "Y");   // fly
}

void Driving(const Gta5Pad& p, nlohmann::json& keys) {
    // Throttle, brake and steering are analogue: gta5.pad.*.
    Press(keys, p.Held(SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER), "Space");
    Press(keys, p.Held(SDL_GAMEPAD_BUTTON_DPAD_RIGHT), "Period");  // radio
    Press(keys, p.Held(SDL_GAMEPAD_BUTTON_DPAD_LEFT), "Comma");
}

void Menu(const Gta5Pad& p, nlohmann::json& keys) {
    Press(keys, p.Held(SDL_GAMEPAD_BUTTON_DPAD_UP) || p.move.y < -0.6f,
          "Up");
    Press(keys, p.Held(SDL_GAMEPAD_BUTTON_DPAD_DOWN) || p.move.y > 0.6f,
          "Down");
    Press(keys, p.Held(SDL_GAMEPAD_BUTTON_DPAD_LEFT) || p.move.x < -0.6f,
          "Left");
    Press(keys, p.Held(SDL_GAMEPAD_BUTTON_DPAD_RIGHT) || p.move.x > 0.6f,
          "Right");
    Press(keys, p.Held(SDL_GAMEPAD_BUTTON_SOUTH), "Return");
    Press(keys, p.Held(SDL_GAMEPAD_BUTTON_EAST), "Backspace");
}

}  // namespace

void PressGta5PadKeys(const Gta5Pad& pad, Gta5PadMode mode,
                      nlohmann::json& keys) {
    if (mode == Gta5PadMode::Menu) {
        Menu(pad, keys);
        return;
    }
    // Everywhere else: Y in and out of cars, Back the camera, Start or
    // (on the map) B the map.
    Press(keys, pad.Held(SDL_GAMEPAD_BUTTON_NORTH), "F");
    Press(keys, pad.Held(SDL_GAMEPAD_BUTTON_BACK), "V");
    Press(keys, pad.Held(SDL_GAMEPAD_BUTTON_START), "Tab");
    if (mode == Gta5PadMode::Map) {
        Press(keys, pad.Held(SDL_GAMEPAD_BUTTON_EAST), "Tab");
    } else if (mode == Gta5PadMode::Driving) {
        Driving(pad, keys);
    } else {
        Foot(pad, keys);
    }
}

}  // namespace sdl3cpp::services::impl
