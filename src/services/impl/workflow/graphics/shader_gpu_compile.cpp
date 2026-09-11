#include "services/interfaces/workflow/graphics/shader_gpu_compile.hpp"

#include <stdexcept>
#include <string>

namespace sdl3cpp::services::impl {

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
    shader_info.num_storage_buffers = params.numStorageBuffers;

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
