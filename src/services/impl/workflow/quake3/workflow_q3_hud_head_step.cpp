#include "services/interfaces/workflow/quake3/workflow_q3_hud_head_step.hpp"
#include "services/interfaces/workflow/rendering/rendering_types.hpp"
#include "services/interfaces/workflow_context.hpp"

#include <SDL3/SDL.h>
#include <SDL3/SDL_gpu.h>
#include <glm/glm.hpp>

#include <string>

namespace sdl3cpp::services::impl {

WorkflowQ3HudHeadStep::WorkflowQ3HudHeadStep(std::shared_ptr<ILogger> l)
    : logger_(std::move(l)) {}

WorkflowQ3HudHeadStep::~WorkflowQ3HudHeadStep() {
    if (device_) {
        if (targets_.color) SDL_ReleaseGPUTexture(device_, targets_.color);
        if (targets_.depth) SDL_ReleaseGPUTexture(device_, targets_.depth);
    }
}

std::string WorkflowQ3HudHeadStep::GetPluginId() const {
    return "q3.hud_head_render";
}

bool WorkflowQ3HudHeadStep::TryInitRT(SDL_GPUDevice* device,
                                      SDL_Window* window) {
    device_  = device;
    targets_ = CreateHeadRenderTargets(device, window, kHeadSz);
    if (logger_) {
        logger_->Info("q3.hud_head_render: render target " +
                      std::string(targets_.ready ? "ready" : "FAILED"));
    }
    return targets_.ready;
}

void WorkflowQ3HudHeadStep::Execute(const WorkflowStepDefinition&,
                                    WorkflowContext& context) {
    // Always clear the tex from last frame so overlay.sw.end never blits a
    // stale portrait (e.g. when the menu is open or the head model isn't
    // loaded).
    context.Set<SDL_GPUTexture*>("overlay.head_gpu_tex", nullptr);

    if (context.GetBool("frame_skip", false)) return;
    if (context.GetBool("q3.menu_open", false)) return;

    auto* device = context.Get<SDL_GPUDevice*>("gpu_device", nullptr);
    auto* cmd =
        context.Get<SDL_GPUCommandBuffer*>("gpu_command_buffer", nullptr);
    if (!device || !cmd) return;

    // Head model must be loaded
    if (context.Get<int>("q3.md3.head_num_surfs", 0) <= 0) return;

    auto* pipeline =
        context.Get<SDL_GPUGraphicsPipeline*>("gpu_pipeline_textured", nullptr);
    if (!pipeline) return;

    auto* window = context.Get<SDL_Window*>("sdl_window", nullptr);
    if (!targets_.ready && !TryInitRT(device, window)) return;

    const HeadAngles angles = UpdateHeadSway(sway_, SDL_GetTicks());
    const glm::mat4 mvp     = BuildHeadPortraitMvp(angles, /*camDist=*/0.45f);
    const glm::mat4 model(1.0f);  // head at world origin
    const rendering::FragmentUniformData fu = DefaultHeadPortraitLighting();

    if (!RenderHeadPortraitPass(cmd, targets_, pipeline, mvp, model, fu,
                                context, kHeadSz)) {
        return;
    }

    // Expose the render target so overlay.sw.end can blit it. Face rect
    // coords are set by q3.hud (which runs after this step).
    context.Set<SDL_GPUTexture*>("overlay.head_gpu_tex", targets_.color);
}

}  // namespace sdl3cpp::services::impl
