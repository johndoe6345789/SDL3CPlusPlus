#include "services/interfaces/workflow/gta5/gta5_gamepad_step.hpp"

#include "services/interfaces/workflow/gta5/gta5_step_params.hpp"
#include "services/interfaces/workflow_context.hpp"

#include <SDL3/SDL_init.h>
#include <SDL3/SDL_timer.h>

#include <algorithm>
#include <utility>

namespace sdl3cpp::services::impl {

WorkflowGta5GamepadStep::WorkflowGta5GamepadStep(
    std::shared_ptr<ILogger> logger, std::shared_ptr<Gta5StreamState> state)
    : logger_(std::move(logger)), state_(std::move(state)) {}

std::string WorkflowGta5GamepadStep::GetPluginId() const {
    return "gta5.gamepad";
}

void WorkflowGta5GamepadStep::Execute(const WorkflowStepDefinition& step,
                                      WorkflowContext& context) {
    if (!started_) started_ = SDL_InitSubSystem(SDL_INIT_GAMEPAD);
    const std::uint64_t now = SDL_GetTicksNS();
    const float dt = lastNs_ ? std::min(float(now - lastNs_) * 1e-9f, 0.1f)
                             : 0.f;
    lastNs_ = now;
    Gta5Pad pad;
    const bool connected = started_ && ReadGta5Pad(gamepad_, pad);
    if (connected != connected_ && logger_) {
        logger_->Info(connected ? std::string("gta5.gamepad: ") +
                                      SDL_GetGamepadName(gamepad_)
                                : "gta5.gamepad: disconnected");
    }
    connected_ = connected;
    const bool driving = state_ && state_->seated >= 0;
    PublishGta5Pad(context, connected ? pad : Gta5Pad{},
                   driving && connected);
    if (!connected) return;

    const bool map = context.GetBool("gta5.map.open", false);
    const Gta5PadMode mode = context.GetBool("gta5.menu.open", false)
                                 ? Gta5PadMode::Menu
                             : map     ? Gta5PadMode::Map
                             : driving ? Gta5PadMode::Driving
                                       : Gta5PadMode::Foot;
    auto keys = context.Get<nlohmann::json>("input.keyboard.state",
                                            nlohmann::json::object());
    if (!keys.is_object()) keys = nlohmann::json::object();
    PressGta5PadKeys(pad, mode, keys);
    context.Set("input.keyboard.state", keys);

    // The right stick turns the view as the mouse does, squared: a light
    // touch aims finely, full lean spins.
    const float speed = Gta5NumberOr(step, "look_speed", 1100.f) * dt;
    const glm::vec2 look = pad.look * glm::abs(pad.look) * speed;
    context.Set<float>("input_mouse_rel_x",
                       context.Get<float>("input_mouse_rel_x", 0.f) + look.x);
    context.Set<float>("input_mouse_rel_y",
                       context.Get<float>("input_mouse_rel_y", 0.f) + look.y);
    // On the map the left stick is the cursor, and A goes there.
    const bool pick = pad.Held(SDL_GAMEPAD_BUTTON_SOUTH);
    context.Set<bool>("gta5.pad.pick", map && pick && !pickHeld_);
    pickHeld_ = pick;
    if (map) MoveGta5PadCursor(context, pad, dt);
}

}  // namespace sdl3cpp::services::impl
