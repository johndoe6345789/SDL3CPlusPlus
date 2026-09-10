#pragma once

#include "services/interfaces/i_workflow_step.hpp"
#include "services/interfaces/i_logger.hpp"

#include <memory>
#include <string>

namespace sdl3cpp::services::impl {

/**
 * @brief Phase 2 of shader system initialization: records glTF model
 * loading configuration.
 *
 * Second step in the `shader.system.initialize` chain (see that step's
 * doc comment). Runs after `shader.system.initialize` and before
 * `shader.system.compile`.
 *
 * Context Output:
 *   - gltf.model_path (std::string)
 *   - gltf.load_status (std::string) = "loaded"
 */
class WorkflowShaderSystemGltfLoadStep : public IWorkflowStep {
public:
    explicit WorkflowShaderSystemGltfLoadStep(
        std::shared_ptr<ILogger> logger);

    std::string GetPluginId() const override;
    void Execute(const WorkflowStepDefinition& step,
                WorkflowContext& context) override;

private:
    std::shared_ptr<ILogger> logger_;
};

}  // namespace sdl3cpp::services::impl
