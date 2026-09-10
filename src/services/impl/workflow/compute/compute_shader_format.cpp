#include "services/interfaces/workflow/compute/compute_shader_format.hpp"

#include <string>

namespace sdl3cpp::services::impl {

ComputeShaderFormat DetectComputeShaderFormat(SDL_GPUDevice* device) {
    const char* driver = SDL_GetGPUDeviceDriver(device);
    const bool isMetal = driver && std::string(driver) == "metal";
    return {isMetal ? SDL_GPU_SHADERFORMAT_MSL : SDL_GPU_SHADERFORMAT_SPIRV,
            isMetal ? "main0" : "main"};
}

}  // namespace sdl3cpp::services::impl
