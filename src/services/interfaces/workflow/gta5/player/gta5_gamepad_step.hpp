#pragma once

#include "services/interfaces/i_logger.hpp"
#include "services/interfaces/i_workflow_step.hpp"
#include "services/interfaces/workflow/gta5/player/gta5_gamepad.hpp"
#include "services/interfaces/workflow/gta5/stream/gta5_stream_state.hpp"

#include <cstdint>
#include <memory>
#include <string>

namespace sdl3cpp::services::impl {

/**
 * Plugin ID: gta5.gamepad
 *
 * An Xbox One pad, as GTA V lays it out (see PressGta5PadKeys). Buttons
 * press the keys the game already reads, in input.keyboard.state; the
 * right stick adds to the mouse's motion (look_speed pixels a second at
 * full lean, squared for fine aim); on the Tab map the left stick moves
 * the cursor and A travels there (gta5.pad.pick). Publishes the
 * analogue rest for whoever reads it: gta5.pad.throttle and .brake (RT,
 * LT, driving), .steer (left stick, driving), .fire and .aim (RT, LT on
 * foot), .look (right stick). Runs after input.keyboard.poll, before
 * everything that reads keys or the mouse.
 */
class WorkflowGta5GamepadStep final : public IWorkflowStep {
public:
    WorkflowGta5GamepadStep(std::shared_ptr<ILogger> logger,
                            std::shared_ptr<Gta5StreamState> state);

    std::string GetPluginId() const override;
    void Execute(const WorkflowStepDefinition& step,
                 WorkflowContext& context) override;

private:
    std::shared_ptr<ILogger> logger_;
    std::shared_ptr<Gta5StreamState> state_;
    SDL_Gamepad* gamepad_{nullptr};
    std::uint64_t lastNs_{0};
    bool started_{false};
    bool connected_{false};
    bool pickHeld_{false};
};

}  // namespace sdl3cpp::services::impl
