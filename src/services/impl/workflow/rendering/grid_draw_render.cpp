#include "services/interfaces/workflow/rendering/grid_draw_render.hpp"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <cstring>
#include <stdexcept>
#include <vector>

namespace sdl3cpp::services::impl {

bool GridGpuResources::IsComplete() const {
    return device && window && pipeline && vertexBuffer && indexBuffer &&
           depthTexture;
}

GridDrawConfig ReadGridDrawConfig(const nlohmann::json& cfg) {
    GridDrawConfig out;
    out.gridWidth  = cfg.value("grid_width", out.gridWidth);
    out.gridHeight = cfg.value("grid_height", out.gridHeight);
    out.spacing    = cfg.value("grid_spacing", out.spacing);
    out.startX     = cfg.value("grid_start_x", out.startX);
    out.startY     = cfg.value("grid_start_y", out.startY);
    out.rotOffsetX = cfg.value("rotation_offset_x", out.rotOffsetX);
    out.rotOffsetY = cfg.value("rotation_offset_y", out.rotOffsetY);
    out.bgR        = cfg.value("background_color_r", out.bgR);
    out.bgG        = cfg.value("background_color_g", out.bgG);
    out.bgB        = cfg.value("background_color_b", out.bgB);
    out.numFrames  = cfg.value("num_frames", out.numFrames);
    return out;
}

void ReadGridCameraMatrices(const WorkflowContext& context, glm::mat4& view,
                            glm::mat4& proj) {
    const std::string cameraKey =
        context.GetString("grid.camera_key", "camera.state");
    const auto* cameraJson =
        context.TryGet<nlohmann::json>(cameraKey);
    if (!cameraJson || !cameraJson->is_object()) {
        throw std::runtime_error(
            "render.grid.draw: camera '" + cameraKey +
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
    gpu.indexBuffer =
        context.Get<SDL_GPUBuffer*>("gpu_index_buffer", nullptr);
    gpu.depthTexture =
        context.Get<SDL_GPUTexture*>("gpu_depth_texture", nullptr);
    return gpu;
}

uint32_t DrawGridCubes(const GridGpuResources& gpu,
                       const GridDrawConfig& cfg, const glm::mat4& view,
                       const glm::mat4& proj, float time) {
    SDL_GPUCommandBuffer* cmd = SDL_AcquireGPUCommandBuffer(gpu.device);
    if (!cmd) return 0;

    SDL_GPUTexture* swapchainTex = nullptr;
    Uint32 sw = 0, sh = 0;
    if (!SDL_WaitAndAcquireGPUSwapchainTexture(cmd, gpu.window,
                                               &swapchainTex, &sw, &sh) ||
        !swapchainTex) {
        SDL_SubmitGPUCommandBuffer(cmd);
        return 0;
    }

    SDL_GPUColorTargetInfo colorTarget = {};
    colorTarget.texture     = swapchainTex;
    colorTarget.clear_color = {cfg.bgR, cfg.bgG, cfg.bgB, 1.0f};
    colorTarget.load_op     = SDL_GPU_LOADOP_CLEAR;
    colorTarget.store_op    = SDL_GPU_STOREOP_STORE;

    SDL_GPUDepthStencilTargetInfo dsTarget = {};
    dsTarget.texture     = gpu.depthTexture;
    dsTarget.clear_depth = 1.0f;
    dsTarget.load_op     = SDL_GPU_LOADOP_CLEAR;
    dsTarget.store_op    = SDL_GPU_STOREOP_DONT_CARE;

    SDL_GPURenderPass* pass =
        SDL_BeginGPURenderPass(cmd, &colorTarget, 1, &dsTarget);
    if (!pass) {
        SDL_SubmitGPUCommandBuffer(cmd);
        return 0;
    }

    SDL_BindGPUGraphicsPipeline(pass, gpu.pipeline);
    SDL_GPUBufferBinding vb = {};
    vb.buffer = gpu.vertexBuffer;
    SDL_BindGPUVertexBuffers(pass, 0, &vb, 1);
    SDL_GPUBufferBinding ib = {};
    ib.buffer = gpu.indexBuffer;
    SDL_BindGPUIndexBuffer(pass, &ib, SDL_GPU_INDEXELEMENTSIZE_16BIT);

    const glm::mat4 viewProj = proj * view;
    struct UniformData { float mvp[16]; };
    uint32_t drawCalls = 0;

    for (uint32_t yy = 0; yy < cfg.gridHeight; ++yy) {
        for (uint32_t xx = 0; xx < cfg.gridWidth; ++xx) {
            const float rotX = time + (static_cast<float>(xx) *
                                       cfg.rotOffsetX);
            const float rotY = time + (static_cast<float>(yy) *
                                       cfg.rotOffsetY);

            glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3(
                cfg.startX + (static_cast<float>(xx) * cfg.spacing),
                cfg.startY + (static_cast<float>(yy) * cfg.spacing),
                0.0f));
            model = glm::rotate(model, rotX, glm::vec3(1.0f, 0.0f, 0.0f));
            model = glm::rotate(model, rotY, glm::vec3(0.0f, 1.0f, 0.0f));

            UniformData uniforms;
            memcpy(uniforms.mvp, glm::value_ptr(viewProj * model),
                  sizeof(uniforms.mvp));
            SDL_PushGPUVertexUniformData(cmd, 0, &uniforms,
                                         sizeof(uniforms));

            SDL_DrawGPUIndexedPrimitives(pass, 36, 1, 0, 0, 0);
            ++drawCalls;
        }
    }

    SDL_EndGPURenderPass(pass);
    SDL_SubmitGPUCommandBuffer(cmd);
    return drawCalls;
}

}  // namespace sdl3cpp::services::impl
