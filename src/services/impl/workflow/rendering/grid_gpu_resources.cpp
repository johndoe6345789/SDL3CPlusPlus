#include "services/interfaces/workflow/rendering/grid_gpu_resources.hpp"

#include <glm/gtc/type_ptr.hpp>
#include <nlohmann/json.hpp>

#include <cstring>
#include <stdexcept>
#include <vector>

namespace sdl3cpp::services::impl {

bool GridGpuResources::IsComplete() const {
    return device && window && pipeline && vertexBuffer && indexBuffer &&
           depthTexture;
}

void ReadGridCameraMatrices(const WorkflowContext& context, glm::mat4& view,
                            glm::mat4& proj) {
    const std::string cameraKey =
        context.GetString("grid.camera_key", "camera.state");
    const auto* cameraJson = context.TryGet<nlohmann::json>(cameraKey);
    if (!cameraJson || !cameraJson->is_object()) {
        throw std::runtime_error("render.grid.draw: camera '" + cameraKey +
                                 "' not found in context");
    }

    auto viewVec = (*cameraJson)["view"].get<std::vector<float>>();
    auto projVec = (*cameraJson)["projection"].get<std::vector<float>>();
    if (viewVec.size() != 16 || projVec.size() != 16) {
        throw std::runtime_error(
            "render.grid.draw: camera matrices must have 16 elements");
    }

    view = glm::mat4(1.0f);
    proj = glm::mat4(1.0f);
    memcpy(glm::value_ptr(view), viewVec.data(), 16 * sizeof(float));
    memcpy(glm::value_ptr(proj), projVec.data(), 16 * sizeof(float));
}

GridGpuResources ReadGridGpuResources(const WorkflowContext& context) {
    GridGpuResources gpu;
    gpu.device = context.Get<SDL_GPUDevice*>("gpu_device", nullptr);
    gpu.window = context.Get<SDL_Window*>("sdl_window", nullptr);
    gpu.pipeline =
        context.Get<SDL_GPUGraphicsPipeline*>("gpu_pipeline", nullptr);
    gpu.vertexBuffer =
        context.Get<SDL_GPUBuffer*>("gpu_vertex_buffer", nullptr);
    gpu.indexBuffer = context.Get<SDL_GPUBuffer*>("gpu_index_buffer", nullptr);
    gpu.depthTexture =
        context.Get<SDL_GPUTexture*>("gpu_depth_texture", nullptr);
    return gpu;
}

}  // namespace sdl3cpp::services::impl
