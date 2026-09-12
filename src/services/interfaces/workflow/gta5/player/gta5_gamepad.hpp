#pragma once

#include "services/interfaces/workflow_context.hpp"

#include <SDL3/SDL_gamepad.h>
#include <glm/glm.hpp>
#include <nlohmann/json.hpp>

#include <array>

namespace sdl3cpp::services::impl {

/// One frame of an Xbox-style pad: sticks past their deadzone, rescaled
/// to reach 1 (y down, as SDL gives it), triggers 0..1, every button.
struct Gta5Pad {
    glm::vec2 move{0.f};  // left stick
    glm::vec2 look{0.f};  // right stick
    float brake{0.f};     // LT
    float throttle{0.f};  // RT
    std::array<bool, SDL_GAMEPAD_BUTTON_COUNT> held{};
    bool Held(SDL_GamepadButton button) const { return held[button]; }
};

/// Opens the first gamepad once one is connected -- closing it when it
/// goes -- and reads it. False with none.
bool ReadGta5Pad(SDL_Gamepad*& gamepad, Gta5Pad& pad);

/// What the pad is driving: the player on foot, a car, the Tab map, or
/// a shop's menu.
enum class Gta5PadMode { Foot, Driving, Map, Menu };

/// The pad pressing the keys the game already reads, by the names
/// input.keyboard.poll gives them, laid out as GTA V lays out its pad.
void PressGta5PadKeys(const Gta5Pad& pad, Gta5PadMode mode,
                      nlohmann::json& keys);

/// The analogue rest: gta5.pad.throttle, .brake and .steer driving,
/// .fire and .aim on foot, .look always.
void PublishGta5Pad(WorkflowContext& context, const Gta5Pad& pad,
                    bool driving);

/// The left stick moving the mouse cursor, over the Tab map.
void MoveGta5PadCursor(WorkflowContext& context, const Gta5Pad& pad,
                       float dt);

}  // namespace sdl3cpp::services::impl
