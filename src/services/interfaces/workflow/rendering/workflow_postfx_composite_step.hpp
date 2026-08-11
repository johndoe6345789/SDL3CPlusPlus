#pragma once

#include "services/interfaces/i_workflow_step.hpp"
#include "services/interfaces/i_logger.hpp"
#include "services/interfaces/workflow_context.hpp"

#include <SDL3/SDL_gpu.h>
#include <SDL3/SDL_render.h>
#include <SDL3/SDL_surface.h>
#include <cstdint>
#include <memory>

namespace sdl3cpp::services::impl {

class WorkflowPostfxCompositeStep final : public IWorkflowStep {
public:
    explicit WorkflowPostfxCompositeStep(std::shared_ptr<ILogger> logger);
    ~WorkflowPostfxCompositeStep();

    std::string GetPluginId() const override;
    void Execute(const WorkflowStepDefinition& step, WorkflowContext& context) override;

private:
    void TryInitOverlay(SDL_GPUDevice* device, WorkflowContext& context);
    void RenderOverlay(SDL_GPUCommandBuffer* cmd, SDL_GPUTexture* swapchain_tex,
                       SDL_GPUDevice* device, WorkflowContext& context);

    std::shared_ptr<ILogger> logger_;

    // FPS overlay — all null until TryInitOverlay succeeds
    bool overlay_ready_   = false;
    bool vbuf_uploaded_   = false;
    SDL_GPUGraphicsPipeline* overlay_pipeline_  = nullptr;
    SDL_GPUTexture*          overlay_tex_       = nullptr;
    SDL_GPUTransferBuffer*   overlay_transfer_  = nullptr;  // text pixels
    SDL_GPUBuffer*           overlay_vtx_buf_   = nullptr;
    SDL_GPUSampler*          overlay_sampler_   = nullptr;
    SDL_Surface*             fps_surface_       = nullptr;
    SDL_Renderer*            fps_renderer_      = nullptr;
    SDL_GPUDevice*           overlay_device_    = nullptr;  // for destructor cleanup

    // FPS smoothing
    float    fps_smooth_ = 0.0f;
    uint64_t fps_last_ns_ = 0;

    static constexpr int kOvW = 160;
    static constexpr int kOvH = 12;
};

}  // namespace sdl3cpp::services::impl
