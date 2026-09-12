#include "services/interfaces/workflow/gta5/gta5_weapon_hold.hpp"

#include <SDL3/SDL_mouse.h>

namespace sdl3cpp::services::impl {

bool Gta5TriggerHeld(WorkflowContext& context) {
    const auto buttons = SDL_GetMouseState(nullptr, nullptr);
    return (buttons & SDL_BUTTON_LMASK) != 0 ||
           context.GetBool("gta5.pad.fire", false);
}

bool Gta5AimHeld(WorkflowContext& context) {
    const auto buttons = SDL_GetMouseState(nullptr, nullptr);
    return (buttons & (SDL_BUTTON_LMASK | SDL_BUTTON_RMASK)) != 0 ||
           context.GetBool("gta5.pad.fire", false) ||
           context.GetBool("gta5.pad.aim", false);
}

}  // namespace sdl3cpp::services::impl
