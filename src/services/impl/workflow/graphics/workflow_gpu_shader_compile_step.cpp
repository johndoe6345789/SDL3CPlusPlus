#include "services/interfaces/workflow/graphics/workflow_gpu_shader_compile_step.hpp"
#include "services/interfaces/workflow/workflow_step_parameter_resolver.hpp"

#include <SDL3/SDL_gpu.h>
#include <nlohmann/json.hpp>
#include <fstream>
#include <cstdlib>
#include <stdexcept>

namespace sdl3cpp::services::impl {

WorkflowGpuShaderCompileStep::WorkflowGpuShaderCompileStep(std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)) {}

std::string WorkflowGpuShaderCompileStep::GetPluginId() const {
    return "graphics.gpu.shader.compile";
}

std::string WorkflowGpuShaderCompileStep::ResolvePath(const std::string& path) {
    if (path.empty() || path[0] != '~') return path;
    const char* home = std::getenv("HOME");
#if defined(_WIN32)
    if (!home) home = std::getenv("USERPROFILE");
#endif
    if (!home) return path;
    return std::string(home) + path.substr(1);
}

std::vector<uint8_t> WorkflowGpuShaderCompileStep::LoadBinary(const std::string& path) {
    std::string resolved = ResolvePath(path);
    std::ifstream file(resolved, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        throw std::runtime_error("graphics.gpu.shader.compile: Failed to open shader file: " + resolved);
    }

    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);

    std::vector<uint8_t> buffer(size);
    if (!file.read(reinterpret_cast<char*>(buffer.data()), size)) {
        throw std::runtime_error("graphics.gpu.shader.compile: Failed to read shader file: " + resolved);
    }

    return buffer;
}

void WorkflowGpuShaderCompileStep::Execute(const WorkflowStepDefinition& step, WorkflowContext& context) {
    WorkflowStepParameterResolver params;

    // Read parameters
    auto getStr = [&](const char* name, const std::string& def) -> std::string {
        const auto* p = params.FindParameter(step, name);
        return (p && p->type == WorkflowParameterValue::Type::String) ? p->stringValue : def;
    };
    auto getInt = [&](const char* name, int def) -> int {
        const auto* p = params.FindParameter(step, name);
        return (p && p->type == WorkflowParameterValue::Type::Number) ? static_cast<int>(p->numberValue) : def;
    };

    std::string shader_path = getStr("shader_path", "");
    const std::string stage_str = getStr("stage", "vertex");
    const int num_uniform_buffers = getInt("num_uniform_buffers", 0);
    const int num_samplers = getInt("num_samplers", 0);
    const std::string output_key = getStr("output_key", "compiled_shader");

    // Fallback: resolve shader_path from inputs (for JSON workflow usage)
    if (shader_path.empty()) {
        auto it = step.inputs.find("shader_path");
        if (it != step.inputs.end() && !it->second.empty()) {
            const auto* pathPtr = context.TryGet<std::string>(it->second);
            if (pathPtr) shader_path = *pathPtr;
        }
    }

    if (shader_path.empty()) {
        throw std::runtime_error("graphics.gpu.shader.compile: 'shader_path' parameter or input is required");
    }

    // Get GPU device
    SDL_GPUDevice* device = context.Get<SDL_GPUDevice*>("gpu_device", nullptr);
    if (!device) {
        throw std::runtime_error("graphics.gpu.shader.compile: GPU device not found in context");
    }

    // Detect shader format from driver
    SDL_GPUShaderFormat format = SDL_GPU_SHADERFORMAT_SPIRV;
    const char* driver = SDL_GetGPUDeviceDriver(device);
    std::string driver_name = driver ? driver : "";
    std::string format_name = "spirv";
    if (driver_name == "metal") {
        format = SDL_GPU_SHADERFORMAT_MSL;
        format_name = "msl";
    } else if (driver_name == "direct3d12") {
        format = SDL_GPU_SHADERFORMAT_DXIL;
        format_name = "dxil";
    }

    // MSL uses "main0" entrypoint, SPIRV uses "main"
    const char* entrypoint = (format == SDL_GPU_SHADERFORMAT_MSL) ? "main0" : "main";

    // Determine shader stage
    SDL_GPUShaderStage stage = SDL_GPU_SHADERSTAGE_VERTEX;
    if (stage_str == "fragment") {
        stage = SDL_GPU_SHADERSTAGE_FRAGMENT;
    }

    // Load shader binary
    auto shader_data = LoadBinary(shader_path);

    // For MSL shaders: ensure null terminator so NSString initWithBytes succeeds
    // on all Metal runtime versions (macOS 26 Metal debug layer can fail otherwise)
    if (format == SDL_GPU_SHADERFORMAT_MSL) {
        shader_data.push_back(0);
    }

    if (logger_) {
        logger_->Info("graphics.gpu.shader.compile: loading " + stage_str +
                      " shader from " + shader_path + " (" +
                      std::to_string(shader_data.size()) + " bytes, format=" + format_name + ")");
    }

    // Create shader
    SDL_GPUShaderCreateInfo shader_info = {};
    shader_info.code = shader_data.data();
    // Pass size WITHOUT the null terminator for SPIRV/METALLIB; for MSL the extra null is harmless
    shader_info.code_size = (format == SDL_GPU_SHADERFORMAT_MSL)
                            ? shader_data.size() - 1   // exclude the appended null
                            : shader_data.size();
    shader_info.entrypoint = entrypoint;
    shader_info.format = format;
    shader_info.stage = stage;
    shader_info.num_uniform_buffers = num_uniform_buffers;
    shader_info.num_samplers = num_samplers;

    SDL_GPUShader* shader = SDL_CreateGPUShader(device, &shader_info);
    if (!shader) {
        throw std::runtime_error("graphics.gpu.shader.compile: Failed to create " + stage_str +
                                 " shader from " + shader_path + ": " + std::string(SDL_GetError()));
    }

    // Store compiled shader in context
    context.Set<SDL_GPUShader*>(output_key, shader);

    // Store metadata as JSON for introspection
    nlohmann::json info;
    info["format"] = format_name;
    info["stage"] = stage_str;
    info["code_size"] = shader_data.size();
    info["entrypoint"] = entrypoint;
    context.Set(output_key + "_info", info);

    if (logger_) {
        logger_->Trace("WorkflowGpuShaderCompileStep", "Execute",
                       "output_key=" + output_key + ", size=" + std::to_string(shader_data.size()),
                       "Shader compiled and stored in context");
    }
}

}  // namespace sdl3cpp::services::impl
