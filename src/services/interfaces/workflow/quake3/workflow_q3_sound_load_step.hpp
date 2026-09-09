#pragma once

#include "services/interfaces/i_logger.hpp"
#include "services/interfaces/i_workflow_step.hpp"

#include <memory>
#include <string>

namespace sdl3cpp::services::impl {

/**
 * Plugin ID: q3.sound.load
 *
 * Decodes the sounds named in a config file out of the pk3 into memory,
 * keyed by their Quake path exactly as ioq3 registers them with
 * trap_S_RegisterSound. Runs once per map load.
 *
 * Reads:  bsp_config (for pk3_path), parameter config_path
 * Writes: q3.sound.bank
 */
class WorkflowQ3SoundLoadStep final : public IWorkflowStep {
public:
    explicit WorkflowQ3SoundLoadStep(std::shared_ptr<ILogger> logger);

    std::string GetPluginId() const override;
    void Execute(const WorkflowStepDefinition& step,
                 WorkflowContext& context) override;

private:
    std::shared_ptr<ILogger> logger_;
};

}  // namespace sdl3cpp::services::impl
