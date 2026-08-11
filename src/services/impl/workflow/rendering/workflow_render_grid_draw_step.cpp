#include "services/interfaces/workflow/rendering/workflow_render_grid_draw_step.hpp"

#include <SDL3/SDL.h>
#include <SDL3/SDL_gpu.h>

#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <nlohmann/json.hpp>
#include <stdexcept>
#include <cstring>
#include <vector>

namespace sdl3cpp::services::impl {

WorkflowRenderGridDrawStep::WorkflowRenderGridDrawStep(
    std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)) {}

std::string WorkflowRenderGridDrawStep::GetPluginId() const {
    return "render.grid.draw";
}

void WorkflowRenderGridDrawStep::Execute(
    const WorkflowStepDefinition& step, WorkflowContext& context) {

    // Read grid config (populated by render.grid.setup)
    const auto* configPtr = context.TryGet<nlohmann::json>("grid.config");
    if (!configPtr || !configPtr->is_object()) {
        throw std::runtime_error("render.grid.draw: grid.config not found (run render.grid.setup first)");
    }
    const auto& cfg = *configPtr;

    const uint32_t gridW = cfg.value("grid_width", 11u), gridH = cfg.value("grid_height", 11u);
    const float spacing = cfg.value("grid_spacing", 3.0f);
    const float startX = cfg.value("grid_start_x", -15.0f), startY = cfg.value("grid_start_y", -15.0f);
    const float rotOffX = cfg.value("rotation_offset_x", 0.21f), rotOffY = cfg.value("rotation_offset_y", 0.37f);
    const float bgR = cfg.value("background_color_r", 0.18f);
    const float bgG = cfg.value("background_color_g", 0.18f), bgB = cfg.value("background_color_b", 0.18f);

    // Read camera matrices
    const std::string cameraKey = context.GetString("grid.camera_key", "camera.state");
    const auto* camera_json = context.TryGet<nlohmann::json>(cameraKey);
    if (!camera_json || !camera_json->is_object()) {
        throw std::runtime_error("render.grid.draw: camera '" + cameraKey + "' not found in context");
    }

    auto view_vec = (*camera_json)["view"].get<std::vector<float>>();
    auto proj_vec = (*camera_json)["projection"].get<std::vector<float>>();
    if (view_vec.size() != 16 || proj_vec.size() != 16) {
        throw std::runtime_error("render.grid.draw: camera matrices must have 16 elements");
    }

    glm::mat4 view(1.0f), proj(1.0f);
    memcpy(glm::value_ptr(view), view_vec.data(), 16 * sizeof(float));
    memcpy(glm::value_ptr(proj), proj_vec.data(), 16 * sizeof(float));

    // Get GPU resources
    SDL_GPUDevice*            device   = context.Get<SDL_GPUDevice*>("gpu_device", nullptr);
    SDL_Window*               window   = context.Get<SDL_Window*>("sdl_window", nullptr);
    SDL_GPUGraphicsPipeline*  pipeline = context.Get<SDL_GPUGraphicsPipeline*>("gpu_pipeline", nullptr);
    SDL_GPUBuffer*            vbuf     = context.Get<SDL_GPUBuffer*>("gpu_vertex_buffer", nullptr);
    SDL_GPUBuffer*            ibuf     = context.Get<SDL_GPUBuffer*>("gpu_index_buffer", nullptr);
    SDL_GPUTexture*           depthTex = context.Get<SDL_GPUTexture*>("gpu_depth_texture", nullptr);

    if (!device || !window || !pipeline || !vbuf || !ibuf || !depthTex) {
        throw std::runtime_error("render.grid.draw: Missing GPU resources (run render.grid.setup first)");
    }

    const float time = static_cast<float>(context.GetDouble("frame.elapsed", 0.0));

    // Acquire command buffer
    SDL_GPUCommandBuffer* cmd = SDL_AcquireGPUCommandBuffer(device);
    if (!cmd) return;

    // Acquire swapchain texture
    SDL_GPUTexture* swapchain_tex = nullptr;
    Uint32 sw = 0, sh = 0;
    if (!SDL_WaitAndAcquireGPUSwapchainTexture(cmd, window, &swapchain_tex, &sw, &sh) || !swapchain_tex) {
        SDL_SubmitGPUCommandBuffer(cmd);
        return;
    }

    // Begin render pass
    SDL_GPUColorTargetInfo color_target = {};
    color_target.texture    = swapchain_tex;
    color_target.clear_color = {bgR, bgG, bgB, 1.0f};
    color_target.load_op    = SDL_GPU_LOADOP_CLEAR;
    color_target.store_op   = SDL_GPU_STOREOP_STORE;

    SDL_GPUDepthStencilTargetInfo ds_target = {};
    ds_target.texture     = depthTex;
    ds_target.clear_depth = 1.0f;
    ds_target.load_op     = SDL_GPU_LOADOP_CLEAR;
    ds_target.store_op    = SDL_GPU_STOREOP_DONT_CARE;

    SDL_GPURenderPass* pass = SDL_BeginGPURenderPass(cmd, &color_target, 1, &ds_target);
    if (!pass) {
        SDL_SubmitGPUCommandBuffer(cmd);
        return;
    }

    // Bind pipeline and vertex/index buffers
    SDL_BindGPUGraphicsPipeline(pass, pipeline);
    SDL_GPUBufferBinding vb = {}; vb.buffer = vbuf;
    SDL_BindGPUVertexBuffers(pass, 0, &vb, 1);
    SDL_GPUBufferBinding ib = {}; ib.buffer = ibuf;
    SDL_BindGPUIndexBuffer(pass, &ib, SDL_GPU_INDEXELEMENTSIZE_16BIT);

    // Per-cube MVP computation and draw submission
    const glm::mat4 viewProj = proj * view;
    struct UniformData { float mvp[16]; };
    uint32_t drawCalls = 0;

    for (uint32_t yy = 0; yy < gridH; ++yy) {
        for (uint32_t xx = 0; xx < gridW; ++xx) {
            const float rotX = time + (static_cast<float>(xx) * rotOffX);
            const float rotY = time + (static_cast<float>(yy) * rotOffY);

            glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3(
                startX + (static_cast<float>(xx) * spacing),
                startY + (static_cast<float>(yy) * spacing), 0.0f));
            model = glm::rotate(model, rotX, glm::vec3(1.0f, 0.0f, 0.0f));
            model = glm::rotate(model, rotY, glm::vec3(0.0f, 1.0f, 0.0f));

            UniformData uniforms;
            memcpy(uniforms.mvp, glm::value_ptr(viewProj * model), sizeof(uniforms.mvp));
            SDL_PushGPUVertexUniformData(cmd, 0, &uniforms, sizeof(uniforms));

            SDL_DrawGPUIndexedPrimitives(pass, 36, 1, 0, 0, 0);
            ++drawCalls;
        }
    }

    SDL_EndGPURenderPass(pass);
    SDL_SubmitGPUCommandBuffer(cmd);

    // Store per-frame stats
    context.Set<uint32_t>("grid.draw_calls", drawCalls);
    context.Set<uint32_t>("grid.cubes_drawn", gridW * gridH);

    // Frame counter management for loop termination
    uint32_t frameNum = context.Get<uint32_t>("frame.number", 0u);
    frameNum++;
    context.Set<uint32_t>("frame.number", frameNum);

    // Advance elapsed time (fixed timestep ~60fps)
    double elapsed = context.GetDouble("frame.elapsed", 0.0);
    context.Set<double>("frame.elapsed", elapsed + (1.0 / 60.0));

    // Check loop termination against num_frames from grid config
    uint32_t numFrames = cfg.value("num_frames", 600u);
    if (frameNum >= numFrames) {
        context.Set<bool>("grid.running", false);
    }

    if (logger_) {
        if (frameNum % 100 == 0) {
            logger_->Trace("WorkflowRenderGridDrawStep", "Execute",
                           "frame=" + std::to_string(frameNum) +
                           ", draw_calls=" + std::to_string(drawCalls));
        }
    }
}

}  // namespace sdl3cpp::services::impl
