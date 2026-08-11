#pragma once

#include "services/interfaces/i_workflow_step.hpp"
#include "services/interfaces/i_logger.hpp"

#include <memory>
#include <string>
#include <vector>

namespace sdl3cpp::services::impl {

/**
 * @brief Atomic step: Load a single shader binary from disk, detect format,
 *        create SDL_GPUShader, store in context.
 *
 * Plugin ID: "graphics.gpu.shader.compile"
 *
 * Parameters:
 *   shader_path          (string)  Path to shader binary (supports ~ expansion)
 *   stage                (string)  "vertex" or "fragment"
 *   num_uniform_buffers  (number)  Uniform buffer count (default: 0)
 *   num_samplers         (number)  Sampler count (default: 0)
 *   output_key           (string)  Context key to store the compiled SDL_GPUShader* (default: "compiled_shader")
 *
 * Requires in context:
 *   "gpu_device" -> SDL_GPUDevice*
 *
 * Stores in context:
 *   <output_key> -> SDL_GPUShader*
 *   <output_key>_info -> nlohmann::json { format, stage, code_size, entrypoint }
 */
class WorkflowGpuShaderCompileStep final : public IWorkflowStep {
public:
    explicit WorkflowGpuShaderCompileStep(std::shared_ptr<ILogger> logger);

    std::string GetPluginId() const override;
    void Execute(const WorkflowStepDefinition& step, WorkflowContext& context) override;

private:
    std::shared_ptr<ILogger> logger_;

    static std::string ResolvePath(const std::string& path);
    static std::vector<uint8_t> LoadBinary(const std::string& path);
};

}  // namespace sdl3cpp::services::impl
