#include "services/interfaces/workflow/graphics/shader_format_detection.hpp"

namespace sdl3cpp::services::impl {

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

}  // namespace sdl3cpp::services::impl
