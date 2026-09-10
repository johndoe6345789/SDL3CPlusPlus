#pragma once

#include "services/interfaces/i_workflow_step.hpp"
#include "services/interfaces/i_logger.hpp"
#include "services/interfaces/workflow_context.hpp"

#include <memory>

namespace sdl3cpp::services::impl {

/**
 * Plugin ID: q3.md3.parse_tags
 *
 * Publishes the MD3's frame/surface/tag counts and its per-frame tag transforms
 * (converted to engine space) for the model read by `q3.md3.read`.
 *
 * Writes to context (prefix from the step's `prefix` parameter):
 *   q3.md3.{prefix}_num_frames  int
 *   q3.md3.{prefix}_num_surfs   int
 *   q3.md3.{prefix}_num_tags    int
 *   q3.md3.{prefix}_tags        nlohmann::json
 */
class WorkflowQ3Md3ParseTagsStep final : public IWorkflowStep {
public:
    explicit WorkflowQ3Md3ParseTagsStep(std::shared_ptr<ILogger> logger);

    std::string GetPluginId() const override;
    void Execute(const WorkflowStepDefinition& step,
                 WorkflowContext& context) override;

private:
    std::shared_ptr<ILogger> logger_;
};

}  // namespace sdl3cpp::services::impl
