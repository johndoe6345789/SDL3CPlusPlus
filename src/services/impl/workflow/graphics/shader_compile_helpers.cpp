#include "services/interfaces/workflow/graphics/shader_compile_helpers.hpp"
#include "services/interfaces/workflow/workflow_step_parameter_resolver.hpp"

#include <cstdlib>
#include <fstream>
#include <stdexcept>

namespace sdl3cpp::services::impl {

std::string ResolveShaderPath(const std::string& path) {
    if (path.empty() || path[0] != '~') return path;
    const char* home = std::getenv("HOME");
#if defined(_WIN32)
    if (!home) home = std::getenv("USERPROFILE");
#endif
    if (!home) return path;
    return std::string(home) + path.substr(1);
}

std::vector<uint8_t> LoadShaderBinary(const std::string& path) {
    const std::string resolved = ResolveShaderPath(path);
    std::ifstream file(resolved, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        throw std::runtime_error(
            "graphics.gpu.shader.compile: Failed to open shader file: " +
            resolved);
    }

    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);

    std::vector<uint8_t> buffer(size);
    if (!file.read(reinterpret_cast<char*>(buffer.data()), size)) {
        throw std::runtime_error(
            "graphics.gpu.shader.compile: Failed to read shader file: " +
            resolved);
    }
    return buffer;
}

ShaderCompileParams ReadShaderCompileParams(const WorkflowStepDefinition& step,
                                            const WorkflowContext& context) {
    WorkflowStepParameterResolver params;

    auto getStr = [&](const char* name, const std::string& def) {
        const auto* p = params.FindParameter(step, name);
        return (p && p->type == WorkflowParameterValue::Type::String)
                   ? p->stringValue
                   : def;
    };
    auto getInt = [&](const char* name, int def) -> int {
        const auto* p = params.FindParameter(step, name);
        return (p && p->type == WorkflowParameterValue::Type::Number)
                   ? static_cast<int>(p->numberValue)
                   : def;
    };

    ShaderCompileParams result;
    result.shaderPath        = getStr("shader_path", "");
    result.stage             = getStr("stage", "vertex");
    result.numUniformBuffers = getInt("num_uniform_buffers", 0);
    result.numSamplers       = getInt("num_samplers", 0);
    result.outputKey         = getStr("output_key", "compiled_shader");

    // Fallback: resolve shader_path from inputs (for JSON workflow usage).
    if (result.shaderPath.empty()) {
        auto it = step.inputs.find("shader_path");
        if (it != step.inputs.end() && !it->second.empty()) {
            const auto* pathPtr = context.TryGet<std::string>(it->second);
            if (pathPtr) result.shaderPath = *pathPtr;
        }
    }
    return result;
}

ShaderFormatInfo DetectShaderFormat(SDL_GPUDevice* device) {
    ShaderFormatInfo info;
    const char* driver            = SDL_GetGPUDeviceDriver(device);
    const std::string driver_name = driver ? driver : "";
    if (driver_name == "metal") {
        info.format     = SDL_GPU_SHADERFORMAT_MSL;
        info.formatName = "msl";
        info.entrypoint = "main0";
    }
    return info;
}

void PrepareShaderBinaryForFormat(const ShaderFormatInfo& formatInfo,
                                  std::vector<uint8_t>& shaderData) {
    // For MSL: ensure null terminator so NSString initWithBytes succeeds on
    // all Metal runtime versions (macOS 26 Metal debug layer can otherwise
    // fail).
    if (formatInfo.format == SDL_GPU_SHADERFORMAT_MSL) {
        shaderData.push_back(0);
    }
}

SDL_GPUShader* CreateCompiledShader(SDL_GPUDevice* device,
                                    const ShaderFormatInfo& formatInfo,
                                    const ShaderCompileParams& params,
                                    const std::vector<uint8_t>& shaderData) {
    SDL_GPUShaderStage stage = SDL_GPU_SHADERSTAGE_VERTEX;
    if (params.stage == "fragment") {
        stage = SDL_GPU_SHADERSTAGE_FRAGMENT;
    }

    SDL_GPUShaderCreateInfo shader_info = {};
    shader_info.code                    = shaderData.data();
    // Pass size WITHOUT the null terminator for SPIRV/METALLIB; for MSL the
    // extra null is harmless.
    shader_info.code_size  = (formatInfo.format == SDL_GPU_SHADERFORMAT_MSL)
                                 ? shaderData.size() - 1
                                 : shaderData.size();
    shader_info.entrypoint = formatInfo.entrypoint;
    shader_info.format     = formatInfo.format;
    shader_info.stage      = stage;
    shader_info.num_uniform_buffers = params.numUniformBuffers;
    shader_info.num_samplers        = params.numSamplers;

    SDL_GPUShader* shader = SDL_CreateGPUShader(device, &shader_info);
    if (!shader) {
        throw std::runtime_error(
            "graphics.gpu.shader.compile: Failed to create " + params.stage +
            " shader from " + params.shaderPath + ": " +
            std::string(SDL_GetError()));
    }
    return shader;
}

}  // namespace sdl3cpp::services::impl
