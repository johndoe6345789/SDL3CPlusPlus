#include "services/interfaces/workflow/rendering/overlay_sw_end_resources_internal.hpp"

namespace sdl3cpp::services::impl::overlay_sw_end_detail {

SDL_GPUGraphicsPipeline* CreateShaderStage(SDL_GPUDevice* device,
                                           SDL_Window* window,
                                           const std::string& vertPath,
                                           const std::string& fragPath) {
    const char* driver           = SDL_GetGPUDeviceDriver(device);
    const std::string driverName = driver ? driver : "";
    const char* entry            = (driverName == "metal") ? "main0" : "main";

    SDL_GPUShaderFormat format = SDL_GPU_SHADERFORMAT_INVALID;
    if (driverName == "metal") {
        format = SDL_GPU_SHADERFORMAT_MSL;
    } else if (driverName == "vulkan") {
        format = SDL_GPU_SHADERFORMAT_SPIRV;
    } else {
        return nullptr;
    }

    const std::vector<uint8_t> vertCode = LoadBinary(vertPath);
    const std::vector<uint8_t> fragCode = LoadBinary(fragPath);
    if (vertCode.empty() || fragCode.empty()) {
        return nullptr;
    }

    SDL_GPUShaderCreateInfo vsi = {};
    vsi.code                    = vertCode.data();
    vsi.code_size               = vertCode.size();
    vsi.entrypoint              = entry;
    vsi.format                  = format;
    vsi.stage                   = SDL_GPU_SHADERSTAGE_VERTEX;

    SDL_GPUShaderCreateInfo fsi = {};
    fsi.code                    = fragCode.data();
    fsi.code_size               = fragCode.size();
    fsi.entrypoint              = entry;
    fsi.format                  = format;
    fsi.stage                   = SDL_GPU_SHADERSTAGE_FRAGMENT;
    fsi.num_samplers            = 1;

    SDL_GPUShader* vertex   = SDL_CreateGPUShader(device, &vsi);
    SDL_GPUShader* fragment = SDL_CreateGPUShader(device, &fsi);
    if (!vertex || !fragment) {
        if (vertex) SDL_ReleaseGPUShader(device, vertex);
        if (fragment) SDL_ReleaseGPUShader(device, fragment);
        return nullptr;
    }

    SDL_GPUGraphicsPipeline* pipeline =
        CreatePipeline(device, window, vertex, fragment);
    SDL_ReleaseGPUShader(device, vertex);
    SDL_ReleaseGPUShader(device, fragment);
    return pipeline;
}

}  // namespace sdl3cpp::services::impl::overlay_sw_end_detail
