#pragma once

#include "services/interfaces/i_logger.hpp"
#include "services/interfaces/i_workflow_step.hpp"
#include "services/interfaces/workflow/gta5/stream/gta5_stream_state.hpp"
#include "services/interfaces/workflow/switchback/session/switchback_session.hpp"

#include <LinearMath/btTransform.h>

#include <memory>
#include <string>

namespace sdl3cpp::services::impl {

/**
 * Plugin ID: switchback.race.restart
 *
 * Puts the car back on the start line when the menu starts a race, or when R
 * is pressed during a race or on the finish screen. The start line is where
 * the car was dropped, captured before the first simulation step moved it.
 * Runs after gta5.vehicle.drop and before the race steps that read the
 * restart flag.
 *
 * Reads:  input.keyboard.state, the session's restart request
 * Writes: switchback.race.restarted (true only on the frame of a restart),
 *         the session's screen (back to the race)
 */
class WorkflowSwitchbackRaceRestartStep final : public IWorkflowStep {
public:
    WorkflowSwitchbackRaceRestartStep(
        std::shared_ptr<ILogger> logger,
        std::shared_ptr<Gta5StreamState> vehicles,
        std::shared_ptr<SwitchbackSession> session);

    std::string GetPluginId() const override;
    void Execute(const WorkflowStepDefinition& step,
                 WorkflowContext& context) override;

private:
    void Reset(Gta5Vehicle& car);

    std::shared_ptr<ILogger> logger_;
    std::shared_ptr<Gta5StreamState> vehicles_;
    std::shared_ptr<SwitchbackSession> session_;
    btTransform start_ = btTransform::getIdentity();
    bool haveStart_ = false;
    bool keyWasDown_ = false;
};

}  // namespace sdl3cpp::services::impl
