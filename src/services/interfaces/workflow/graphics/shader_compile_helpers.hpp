#pragma once

#include "services/interfaces/workflow_context.hpp"
#include "services/interfaces/workflow_step_definition.hpp"

#include <SDL3/SDL_gpu.h>

#include <cstdint>
#include <string>
#include <vector>

namespace sdl3cpp::services::impl {

/// Expands a leading `~` to `$HOME` (`$USERPROFILE` on Windows if `$HOME`
/// isn't set); a no-op if neither is set or `path` doesn't start with `~`.
std::string ResolveShaderPath(const std::string& path);

/// Reads the whole file at `path` (after `~`-expansion) into memory; throws
/// std::runtime_error (prefixed "graphics.gpu.shader.compile: ...") if it
/// can't be opened or read.
std::vector<uint8_t> LoadShaderBinary(const std::string& path);

/// graphics.gpu.shader.compile's resolved parameters, each with the
/// original defaults; `shaderPath` falls back to a wired context input
/// (`step.inputs["shader_path"]`) when the parameter itself is empty.
struct ShaderCompileParams {
    std::string shaderPath;
    std::string stage     = "vertex";
    int numUniformBuffers = 0;
    int numSamplers       = 0;
    std::string outputKey = "compiled_shader";
};

ShaderCompileParams ReadShaderCompileParams(const WorkflowStepDefinition& step,
                                            const WorkflowContext& context);

/// The GPU shader format/entrypoint this device's driver expects: SPIR-V
/// with entrypoint "main" everywhere except Metal, which uses MSL with
/// entrypoint "main0".
struct ShaderFormatInfo {
    SDL_GPUShaderFormat format = SDL_GPU_SHADERFORMAT_SPIRV;
    std::string formatName     = "spirv";
    const char* entrypoint     = "main";
};

ShaderFormatInfo DetectShaderFormat(SDL_GPUDevice* device);

/// For MSL, appends a null terminator to `shaderData` (Metal's NSString
/// initWithBytes can otherwise fail on some runtime versions); a no-op for
/// every other format. Call once, right after loading the binary.
void PrepareShaderBinaryForFormat(const ShaderFormatInfo& formatInfo,
                                  std::vector<uint8_t>& shaderData);

/**
 * @brief Creates the compiled SDL_GPUShader for one shader stage.
 *
 * `shaderData` must already have been passed through
 * PrepareShaderBinaryForFormat. Excludes MSL's appended null terminator
 * from `code_size`; SPIR-V uses `shaderData.size()` as-is. Throws
 * std::runtime_error (prefixed "graphics.gpu.shader.compile: ...") if
 * shader creation fails.
 */
SDL_GPUShader* CreateCompiledShader(SDL_GPUDevice* device,
                                    const ShaderFormatInfo& formatInfo,
                                    const ShaderCompileParams& params,
                                    const std::vector<uint8_t>& shaderData);

}  // namespace sdl3cpp::services::impl
