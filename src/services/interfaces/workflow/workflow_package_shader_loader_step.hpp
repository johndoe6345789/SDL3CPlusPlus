#pragma once

#include "services/interfaces/i_workflow_step.hpp"
#include "services/interfaces/i_logger.hpp"
#include "services/interfaces/workflow_definition.hpp"

#include <filesystem>
#include <memory>
#include <string>

namespace sdl3cpp::services::impl {

/**
 * Workflow step that loads shader references from package.json metadata
 * and stores them in the workflow context for use by the shader compiler.
 *
 * Usage: Reads packages/{gamePackage}/package.json and extracts shader paths
 * from the "shaders" array, storing shader vertex/fragment paths in context
 * variables for downstream consumption.
 *
 * Context output:
 * - shader.vertex_path: Path to vertex shader file
 * - shader.fragment_path: Path to fragment shader file
 * - shader.id: Shader identifier
 * - shader.useConstantColor: Whether to use constant color mode
 * - shader.constantColor: RGBA color values
 */
class WorkflowPackageShaderLoaderStep : public IWorkflowStep {
 public:
    WorkflowPackageShaderLoaderStep(
        std::shared_ptr<ILogger> logger,
        const std::string& gamePackage,
        const std::filesystem::path& projectRoot);

    std::string GetPluginId() const override;
    void Execute(const WorkflowStepDefinition& step, WorkflowContext& context) override;

 private:
    std::shared_ptr<ILogger> logger_;
    std::string gamePackage_;
    std::filesystem::path projectRoot_;

    std::filesystem::path ResolvePackageRoot(const std::filesystem::path& projectRoot);
};

}  // namespace sdl3cpp::services::impl
