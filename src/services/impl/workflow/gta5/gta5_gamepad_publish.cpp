#include "services/interfaces/workflow/gta5/gta5_gamepad.hpp"

#include <SDL3/SDL_mouse.h>
#include <SDL3/SDL_video.h>

namespace sdl3cpp::services::impl {

void PublishGta5Pad(WorkflowContext& context, const Gta5Pad& pad,
                    bool driving) {
    context.Set<float>("gta5.pad.throttle", driving ? pad.throttle : 0.f);
    context.Set<float>("gta5.pad.brake", driving ? pad.brake : 0.f);
    context.Set<float>("gta5.pad.steer", driving ? -pad.move.x : 0.f);
    context.Set<bool>("gta5.pad.fire", !driving && pad.throttle > 0.5f);
    context.Set<bool>("gta5.pad.aim", !driving && pad.brake > 0.5f);
    context.Set<glm::vec2>("gta5.pad.look", pad.look);
}

void MoveGta5PadCursor(WorkflowContext& context, const Gta5Pad& pad,
                       float dt) {
    auto* window = context.Get<SDL_Window*>("sdl_window", nullptr);
    if (!window || glm::length(pad.move) <= 0.f) return;
    float x = 0.f, y = 0.f;
    SDL_GetMouseState(&x, &y);
    const glm::vec2 to = glm::vec2(x, y) + pad.move * 600.f * dt;
    SDL_WarpMouseInWindow(window, to.x, to.y);
}

}  // namespace sdl3cpp::services::impl
