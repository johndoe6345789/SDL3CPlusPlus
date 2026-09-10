#include "services/interfaces/workflow/graphics/workflow_gpu_shader_compile_step.hpp"
#include "services/interfaces/workflow/graphics/shader_binary_io.hpp"
#include "services/interfaces/workflow/graphics/shader_compile_params.hpp"
#include "services/interfaces/workflow/graphics/shader_format_detection.hpp"
#include "services/interfaces/workflow/graphics/shader_gpu_compile.hpp"

#include <SDL3/SDL_gpu.h>
#include <nlohmann/json.hpp>

#include <stdexcept>

namespace sdl3cpp::services::impl {

WorkflowGpuShaderCompileStep::WorkflowGpuShaderCompileStep(
    std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)) {}

std::string WorkflowGpuShaderCompileStep::GetPluginId() const {
    return "graphics.gpu.shader.compile";
}

void WorkflowGpuShaderCompileStep::Execute(const WorkflowStepDefinition& step,
                                           WorkflowContext& context) {
    const ShaderCompileParams params = ReadShaderCompileParams(step, context);
    if (params.shaderPath.empty()) {
        throw std::runtime_error(
            "graphics.gpu.shader.compile: 'shader_path' parameter or input "
            "is required");
    }

    auto* device = context.Get<SDL_GPUDevice*>("gpu_device", nullptr);
    if (!device) {
        throw std::runtime_error(
            "graphics.gpu.shader.compile: GPU device not found in context");
    }

    const ShaderFormatInfo formatInfo = DetectShaderFormat(device);
    auto shader_data                  = LoadShaderBinary(params.shaderPath);
    PrepareShaderBinaryForFormat(formatInfo, shader_data);

    if (logger_) {
        logger_->Info("graphics.gpu.shader.compile: loading " + params.stage +
                      " shader from " + params.shaderPath + " (" +
                      std::to_string(shader_data.size()) +
                      " bytes, format=" + formatInfo.formatName + ")");
    }

    SDL_GPUShader* shader =
        CreateCompiledShader(device, formatInfo, params, shader_data);

    context.Set<SDL_GPUShader*>(params.outputKey, shader);

    nlohmann::json info;
    info["format"]     = formatInfo.formatName;
    info["stage"]      = params.stage;
    info["code_size"]  = shader_data.size();
    info["entrypoint"] = formatInfo.entrypoint;
    context.Set(params.outputKey + "_info", info);

    if (logger_) {
        logger_->Trace("WorkflowGpuShaderCompileStep", "Execute",
                       "output_key=" + params.outputKey +
                           ", size=" + std::to_string(shader_data.size()),
                       "Shader compiled and stored in context");
    }
}

}  // namespace sdl3cpp::services::impl
