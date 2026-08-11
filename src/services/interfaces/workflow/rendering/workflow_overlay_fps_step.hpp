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

class WorkflowOverlayFpsStep final : public IWorkflowStep {
public:
    explicit WorkflowOverlayFpsStep(std::shared_ptr<ILogger> logger);
    ~WorkflowOverlayFpsStep();

    std::string GetPluginId() const override;
    void Execute(const WorkflowStepDefinition& step, WorkflowContext& context) override;

private:
    void TryInit(SDL_GPUDevice* device, SDL_Window* window);
    void Render(SDL_GPUCommandBuffer* cmd, SDL_GPUTexture* swapchain_tex,
                SDL_GPUDevice* device, uint32_t frame_w, uint32_t frame_h);

    std::shared_ptr<ILogger> logger_;

    bool     ready_         = false;
    bool     disabled_      = false;
    bool     vbuf_uploaded_ = false;
    SDL_GPUGraphicsPipeline* pipeline_ = nullptr;
    SDL_GPUTexture*          tex_      = nullptr;
    SDL_GPUTransferBuffer*   transfer_ = nullptr;
    SDL_GPUBuffer*           vtx_buf_  = nullptr;
    SDL_GPUSampler*          sampler_  = nullptr;
    SDL_Surface*             surface_  = nullptr;
    SDL_Renderer*            renderer_ = nullptr;
    SDL_GPUDevice*           device_   = nullptr;

    float    fps_smooth_ = 0.0f;
    uint64_t fps_last_ns_ = 0;

    static constexpr int kW = 160;
    static constexpr int kH = 12;
};

}  // namespace sdl3cpp::services::impl
