#pragma once
#include "services/interfaces/i_workflow_step.hpp"
#include "services/interfaces/i_logger.hpp"
#include "services/interfaces/workflow_context.hpp"
#include <SDL3/SDL_gpu.h>
#include <memory>
#include <string>

namespace sdl3cpp::services::impl {
class WorkflowOverlaySwEndStep final : public IWorkflowStep {
public:
    explicit WorkflowOverlaySwEndStep(std::shared_ptr<ILogger> logger);
    ~WorkflowOverlaySwEndStep();
    std::string GetPluginId() const override;
    void Execute(const WorkflowStepDefinition& step, WorkflowContext& context) override;
private:
    void TryInit(SDL_GPUDevice* device, SDL_Window* window,
                 const std::string& vertPath, const std::string& fragPath);
    std::shared_ptr<ILogger> logger_;
    void BlitHeadPortrait(SDL_GPUCommandBuffer* cmd, SDL_GPUTexture* swapchain,
                          SDL_GPUTexture* headTex, WorkflowContext& context);

    bool ready_         = false;
    bool disabled_      = false;
    bool vbuf_uploaded_ = false;
    SDL_GPUDevice*           device_   = nullptr;
    SDL_GPUGraphicsPipeline* pipeline_ = nullptr;
    SDL_GPUTexture*          tex_      = nullptr;
    SDL_GPUTransferBuffer*   transfer_ = nullptr;
    SDL_GPUBuffer*           vtx_buf_  = nullptr;
    SDL_GPUSampler*          sampler_  = nullptr;
    // Head portrait blit resources (created lazily, same pipeline/sampler)
    SDL_GPUBuffer*           head_vtx_buf_  = nullptr;
    SDL_GPUSampler*          head_sampler_  = nullptr;
    static constexpr int kW = 640, kH = 360;
};
} // namespace sdl3cpp::services::impl
