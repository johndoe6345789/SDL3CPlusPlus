#pragma once

#include "services/interfaces/i_workflow_step.hpp"
#include "services/interfaces/i_logger.hpp"

#include <memory>

namespace sdl3cpp::services::impl {

/**
 * Plugin ID: postfx.bloom_extract
 *
 * Extracts bright pixels from the HDR scene texture into a half-res
 * ping/pong texture pair (creating/resizing them as needed), for the
 * bloom blur pass to read. See bloom_ping_pong.hpp for the texture
 * management and draw logic.
 */
class WorkflowPostfxBloomExtractStep final : public IWorkflowStep {
public:
    explicit WorkflowPostfxBloomExtractStep(std::shared_ptr<ILogger> logger);

    std::string GetPluginId() const override;
    void Execute(const WorkflowStepDefinition& step,
                 WorkflowContext& context) override;

private:
    std::shared_ptr<ILogger> logger_;
};

}  // namespace sdl3cpp::services::impl
