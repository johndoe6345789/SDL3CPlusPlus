#pragma once

#include <SDL3/SDL_gpu.h>

#include <cstdint>
#include <string>
#include <vector>

namespace sdl3cpp::services::impl {

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

}  // namespace sdl3cpp::services::impl
