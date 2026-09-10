#pragma once

#include "services/interfaces/i_workflow_step.hpp"
#include "services/interfaces/i_logger.hpp"
#include "services/interfaces/workflow/quake3/q3_md3_source.hpp"

#include <memory>
#include <string>
#include <unordered_map>

namespace sdl3cpp::services::impl {

/**
 * Plugin ID: q3.md3.read
 *
 * Reads one MD3 (and its optional .skin) out of the map's pk3 and publishes the
 * bytes for the q3.md3.parse_* and q3.md3.upload_surfaces steps that follow.
 *
 * Parameters: prefix, path, skin, anim (see ReadQ3Md3StepParameters).
 * Reads from context: bsp_config (for pk3_path), gpu_device.
 * Writes to context: q3.md3.{prefix}_source -> Q3Md3Source*
 *
 * Idempotent per prefix: a model that has already been read is not read again.
 */
class WorkflowQ3Md3ReadStep final : public IWorkflowStep {
public:
    explicit WorkflowQ3Md3ReadStep(std::shared_ptr<ILogger> logger);

    std::string GetPluginId() const override;
    void Execute(const WorkflowStepDefinition& step,
                 WorkflowContext& context) override;

private:
    std::shared_ptr<ILogger> logger_;
    std::unordered_map<std::string, Q3Md3Source> sources_;
};

}  // namespace sdl3cpp::services::impl
