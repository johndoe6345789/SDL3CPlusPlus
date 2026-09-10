#include "services/interfaces/workflow/graphics/gpu_pipeline_shaders.hpp"

#include <stdexcept>

namespace sdl3cpp::services::impl {

GpuPipelineShaders RequireGpuPipelineShaders(WorkflowContext& context,
                                             const GpuPipelineCreateParams& p) {
    GpuPipelineShaders shaders;
    shaders.vertex = context.Get<SDL_GPUShader*>(p.vertexShaderKey, nullptr);
    shaders.fragment =
        context.Get<SDL_GPUShader*>(p.fragmentShaderKey, nullptr);
    if (!shaders.vertex) {
        throw std::runtime_error(
            "graphics.gpu.pipeline.create: Vertex shader not found at "
            "key '" +
            p.vertexShaderKey + "'");
    }
    if (!shaders.fragment) {
        throw std::runtime_error(
            "graphics.gpu.pipeline.create: Fragment shader not found at "
            "key '" +
            p.fragmentShaderKey + "'");
    }
    return shaders;
}

}  // namespace sdl3cpp::services::impl
