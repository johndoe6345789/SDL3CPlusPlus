#include "services/interfaces/workflow/graphics/workflow_graphics_gpu_init_step.hpp"
#include "services/interfaces/workflow/graphics/gpu_device_create.hpp"
#include "services/interfaces/workflow/graphics/gpu_device_window.hpp"
#include "services/interfaces/workflow/graphics/gpu_present_mode.hpp"
#include "services/interfaces/workflow/workflow_step_io_resolver.hpp"

#include <SDL3/SDL_gpu.h>
#include <SDL3/SDL.h>
#include <nlohmann/json.hpp>
#include <cstdlib>
#include <stdexcept>

namespace sdl3cpp::services::impl {

WorkflowGraphicsGpuInitStep::WorkflowGraphicsGpuInitStep(
    std::shared_ptr<ILogger> logger,
    std::shared_ptr<IPlatformService> platform_service)
    : logger_(std::move(logger)),
      platform_service_(std::move(platform_service)) {}

std::string WorkflowGraphicsGpuInitStep::GetPluginId() const {
    return "graphics.gpu.init";
}

void WorkflowGraphicsGpuInitStep::Execute(const WorkflowStepDefinition& step,
                                          WorkflowContext& context) {
    WorkflowStepIoResolver resolver;
    const std::string viewportKey =
        resolver.GetRequiredInputKey(step, "viewport_config");
    const std::string rendererKey =
        resolver.GetRequiredInputKey(step, "selected_renderer");
    const std::string outputHandleKey =
        resolver.GetRequiredOutputKey(step, "gpu_handle");

    const auto* viewport_config = context.TryGet<nlohmann::json>(viewportKey);
    const auto* renderer_str    = context.TryGet<std::string>(rendererKey);

    if (!viewport_config || !renderer_str) {
        throw std::runtime_error(
            "graphics.gpu.init requires "
            "viewport_config and selected_renderer inputs");
    }

    uint32_t width       = (*viewport_config)["width"];
    uint32_t height      = (*viewport_config)["height"];
    std::string renderer = *renderer_str;

    // Debug mode default off (SDL_GPU_DEBUG=1 opts in); see
    // CreateGpuDeviceWithFallback's doc comment for why.
    const bool debugMode = std::getenv("SDL_GPU_DEBUG") != nullptr;

    SDL_GPUDevice* device =
        CreateGpuDeviceWithFallback(logger_, renderer, debugMode);

    SDL_Window* window = context.Get<SDL_Window*>("sdl_window", nullptr);
    ClaimWindowForGpuOrThrow(device, window);

    // Read from viewport_config so present-mode override stays
    // JSON-driven without needing a new step.
    ApplyPresentModeOverride(device, window, *viewport_config, logger_);

    const char* device_driver = SDL_GetGPUDeviceDriver(device);
    if (logger_) {
        logger_->Trace("WorkflowGraphicsGpuInitStep", "Execute",
                       DescribeGpuInit(width, height, device),
                       "GPU device initialized successfully");
    }

    // Store GPU device pointer in context for all downstream steps,
    // plus state as JSON for compatibility.
    context.Set<SDL_GPUDevice*>("gpu_device", device);
    nlohmann::json gpu_state = {
        {"initialized", true},
        {"width", width},
        {"height", height},
        {"renderer", device_driver ? device_driver : renderer}};
    context.Set(outputHandleKey, gpu_state);
}

}  // namespace sdl3cpp::services::impl
