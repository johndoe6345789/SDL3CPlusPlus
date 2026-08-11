#include "services/interfaces/workflow/graphics/workflow_graphics_frame_begin_step.hpp"
#include "services/interfaces/workflow/workflow_step_io_resolver.hpp"

#include <SDL3/SDL_gpu.h>
#include <nlohmann/json.hpp>
#include <stdexcept>
#include <chrono>

namespace sdl3cpp::services::impl {

WorkflowGraphicsFrameBeginStep::WorkflowGraphicsFrameBeginStep(
    std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)) {}

std::string WorkflowGraphicsFrameBeginStep::GetPluginId() const {
    return "graphics.frame.begin";
}

void WorkflowGraphicsFrameBeginStep::Execute(
    const WorkflowStepDefinition& step, WorkflowContext& context) {
    WorkflowStepIoResolver resolver;
    const std::string clearColorKey = resolver.GetRequiredInputKey(step, "clear_color");
    const std::string outputFrameKey = resolver.GetRequiredOutputKey(step, "frame_id");

    const auto* clear_color_json = context.TryGet<nlohmann::json>(clearColorKey);
    if (!clear_color_json || !clear_color_json->is_array() || clear_color_json->size() != 4) {
        throw std::runtime_error("graphics.frame.begin requires clear_color input (array of 4 floats [r,g,b,a])");
    }

    float r = (*clear_color_json)[0].get<float>();
    float g = (*clear_color_json)[1].get<float>();
    float b = (*clear_color_json)[2].get<float>();
    float a = (*clear_color_json)[3].get<float>();

    SDL_GPUDevice* device = context.Get<SDL_GPUDevice*>("gpu_device", nullptr);
    SDL_Window* window = context.Get<SDL_Window*>("sdl_window", nullptr);
    if (!device || !window) {
        throw std::runtime_error("graphics.frame.begin: GPU device or SDL window not found in context");
    }

    // Acquire command buffer for this frame
    SDL_GPUCommandBuffer* cmd = SDL_AcquireGPUCommandBuffer(device);
    if (!cmd) {
        throw std::runtime_error("graphics.frame.begin: SDL_AcquireGPUCommandBuffer failed: " +
                                 std::string(SDL_GetError()));
    }

    // Acquire swapchain texture
    SDL_GPUTexture* swapchain_texture = nullptr;
    Uint32 swapchain_w = 0, swapchain_h = 0;
    if (!SDL_WaitAndAcquireGPUSwapchainTexture(cmd, window, &swapchain_texture, &swapchain_w, &swapchain_h)) {
        SDL_CancelGPUCommandBuffer(cmd);
        throw std::runtime_error("graphics.frame.begin: SDL_WaitAndAcquireGPUSwapchainTexture failed: " +
                                 std::string(SDL_GetError()));
    }

    if (!swapchain_texture) {
        // Window minimized or not visible - submit empty command buffer
        SDL_SubmitGPUCommandBuffer(cmd);

        static uint32_t frame_counter = 0;
        nlohmann::json frame_data = {{"frame_id", frame_counter++}, {"skipped", true}};
        context.Set(outputFrameKey, frame_data);
        return;
    }

    // Store command buffer and swapchain texture for render pass and frame end
    context.Set<SDL_GPUCommandBuffer*>("gpu_cmd", cmd);
    context.Set<SDL_GPUTexture*>("gpu_swapchain_texture", swapchain_texture);

    // Get or create depth texture
    SDL_GPUTexture* depth_texture = context.Get<SDL_GPUTexture*>("gpu_depth_texture", nullptr);
    if (!depth_texture) {
        SDL_GPUTextureCreateInfo depth_info = {};
        depth_info.type = SDL_GPU_TEXTURETYPE_2D;
        depth_info.format = SDL_GPU_TEXTUREFORMAT_D32_FLOAT;
        depth_info.width = swapchain_w;
        depth_info.height = swapchain_h;
        depth_info.layer_count_or_depth = 1;
        depth_info.num_levels = 1;
        depth_info.usage = SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET;

        depth_texture = SDL_CreateGPUTexture(device, &depth_info);
        if (!depth_texture) {
            throw std::runtime_error("graphics.frame.begin: Failed to create depth texture: " +
                                     std::string(SDL_GetError()));
        }
        context.Set<SDL_GPUTexture*>("gpu_depth_texture", depth_texture);
    }

    // Begin render pass with clear color
    SDL_GPUColorTargetInfo color_target = {};
    color_target.texture = swapchain_texture;
    color_target.clear_color.r = r;
    color_target.clear_color.g = g;
    color_target.clear_color.b = b;
    color_target.clear_color.a = a;
    color_target.load_op = SDL_GPU_LOADOP_CLEAR;
    color_target.store_op = SDL_GPU_STOREOP_STORE;

    SDL_GPUDepthStencilTargetInfo depth_target = {};
    depth_target.texture = depth_texture;
    depth_target.clear_depth = 1.0f;
    depth_target.load_op = SDL_GPU_LOADOP_CLEAR;
    depth_target.store_op = SDL_GPU_STOREOP_DONT_CARE;

    SDL_GPURenderPass* render_pass = SDL_BeginGPURenderPass(cmd, &color_target, 1, &depth_target);
    if (!render_pass) {
        throw std::runtime_error("graphics.frame.begin: SDL_BeginGPURenderPass failed: " +
                                 std::string(SDL_GetError()));
    }

    context.Set<SDL_GPURenderPass*>("gpu_render_pass", render_pass);

    if (logger_) {
        logger_->Trace("WorkflowGraphicsFrameBeginStep", "Execute",
                       "clear_color=(" + std::to_string(r) + "," + std::to_string(g) +
                       "," + std::to_string(b) + "," + std::to_string(a) + ")" +
                       ", swapchain=" + std::to_string(swapchain_w) + "x" + std::to_string(swapchain_h),
                       "Frame begin: render pass started");
    }

    // Store frame metadata
    static uint32_t frame_counter = 0;
    nlohmann::json frame_data = {
        {"frame_id", frame_counter++},
        {"clear_color", *clear_color_json},
        {"skipped", false},
        {"timestamp", static_cast<double>(std::chrono::high_resolution_clock::now().time_since_epoch().count())}
    };
    context.Set(outputFrameKey, frame_data);
}

}  // namespace sdl3cpp::services::impl
