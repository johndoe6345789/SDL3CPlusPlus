#include "services/interfaces/workflow/rendering/workflow_debug_screenshot_step.hpp"
#include "services/interfaces/workflow/workflow_step_parameter_resolver.hpp"

#include <SDL3/SDL_surface.h>
#include <SDL3/SDL_gpu.h>

#include <SDL3/SDL_events.h>

#include <cstdio>
#include <fstream>
#include <string>

namespace sdl3cpp::services::impl {

WorkflowDebugScreenshotStep::WorkflowDebugScreenshotStep(std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)) {}

std::string WorkflowDebugScreenshotStep::GetPluginId() const {
    return "debug.screenshot";
}

void WorkflowDebugScreenshotStep::Execute(
    const WorkflowStepDefinition& step, WorkflowContext& context) {

    if (saved_) return;
    saved_ = true;

    // Write status file so we can see exactly what state everything is in
    {
        std::ofstream f("debug_screenshot_status.txt");
        f << "frame_skip=" << context.GetBool("frame_skip", false) << "\n";
        f << "overlay_fps_ready=" << context.GetBool("overlay_fps_ready", false) << "\n";
        f << "has_cmd=" << (context.Get<SDL_GPUCommandBuffer*>("gpu_command_buffer", nullptr) != nullptr) << "\n";
        f << "has_swapchain=" << (context.Get<SDL_GPUTexture*>("gpu_swapchain_texture", nullptr) != nullptr) << "\n";
        f << "has_device=" << (context.Get<SDL_GPUDevice*>("gpu_device", nullptr) != nullptr) << "\n";
        f << "has_window=" << (context.Get<SDL_Window*>("sdl_window", nullptr) != nullptr) << "\n";

        auto* device = context.Get<SDL_GPUDevice*>("gpu_device", nullptr);
        if (device) {
            const char* drv = SDL_GetGPUDeviceDriver(device);
            f << "gpu_driver=" << (drv ? drv : "null") << "\n";
        }

        f << "frame_width="  << context.Get<uint32_t>("frame_width",  0u) << "\n";
        f << "frame_height=" << context.Get<uint32_t>("frame_height", 0u) << "\n";
    }

    // Save the overlay SDL surface as BMP — confirms text rasterisation is working
    auto* surface = context.Get<SDL_Surface*>("overlay_fps_surface", nullptr);
    if (surface) {
        SDL_SaveBMP(surface, "debug_fps_overlay.bmp");
        if (logger_) logger_->Info("debug.screenshot: saved debug_fps_overlay.bmp");
    } else {
        if (logger_) logger_->Warn("debug.screenshot: overlay_fps_surface not in context");
    }

    if (logger_) logger_->Info("debug.screenshot: wrote debug_screenshot_status.txt");

    // Push a quit event so the game exits automatically after saving
    SDL_Event quit;
    quit.type = SDL_EVENT_QUIT;
    SDL_PushEvent(&quit);
}

}  // namespace sdl3cpp::services::impl
