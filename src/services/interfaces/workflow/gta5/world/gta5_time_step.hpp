#pragma once

#include "services/interfaces/i_logger.hpp"
#include "services/interfaces/i_workflow_step.hpp"

#include <cstdint>
#include <memory>
#include <string>

namespace sdl3cpp::services::impl {

/**
 * Plugin ID: gta5.time
 *
 * The clock and the day it drives. Starts at start_hour and runs
 * minutes_per_second game minutes each real second; holding T runs it
 * sixty times as fast. Each frame it rewrites lighting.directional -- the
 * sun by day, the moon by night -- and publishes the sky's colours
 * (gta5.time.horizon, gta5.time.zenith), how dark it is (gta5.time.night)
 * and the time as HH:MM (gta5.clock.text). Runs before render.prepare.
 */
class WorkflowGta5TimeStep final : public IWorkflowStep {
public:
    explicit WorkflowGta5TimeStep(std::shared_ptr<ILogger> logger);

    std::string GetPluginId() const override;
    void Execute(const WorkflowStepDefinition& step,
                 WorkflowContext& context) override;

private:
    std::shared_ptr<ILogger> logger_;
    float hours_{-1.f};
    std::uint64_t lastMs_{0};
    int shownHour_{-1};
};

}  // namespace sdl3cpp::services::impl
