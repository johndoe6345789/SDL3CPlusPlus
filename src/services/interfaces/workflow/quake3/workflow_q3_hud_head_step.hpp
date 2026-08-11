#pragma once
#include "services/interfaces/i_workflow_step.hpp"
#include "services/interfaces/i_logger.hpp"
#include "services/interfaces/workflow_context.hpp"
#include <SDL3/SDL_gpu.h>
#include <glm/glm.hpp>
#include <memory>
#include <string>

namespace sdl3cpp::services::impl {

// Renders the player head MD3 model to a small (kHeadSz x kHeadSz) GPU render
// target each frame and stores it in context as "overlay.head_gpu_tex".
// overlay.sw.end reads that texture and blits it at "hud.face_rect_*" position.
class WorkflowQ3HudHeadStep final : public IWorkflowStep {
public:
    explicit WorkflowQ3HudHeadStep(std::shared_ptr<ILogger> logger);
    ~WorkflowQ3HudHeadStep();
    std::string GetPluginId() const override;
    void Execute(const WorkflowStepDefinition& step, WorkflowContext& context) override;

    static constexpr int kHeadSz = 64;  // square render target size

private:
    bool TryInitRT(SDL_GPUDevice* device, SDL_Window* window);

    std::shared_ptr<ILogger> logger_;
    SDL_GPUDevice*  device_   = nullptr;
    SDL_GPUTexture* color_rt_ = nullptr;
    SDL_GPUTexture* depth_rt_ = nullptr;
    bool            ready_    = false;

    // Q3A-style idle sway state (mirrors cg.headStart/EndYaw/Pitch/Time)
    float  swayStartYaw_   = glm::radians(180.f);
    float  swayStartPitch_ = 0.f;
    float  swayEndYaw_     = glm::radians(180.f);
    float  swayEndPitch_   = 0.f;
    uint64_t swayStartMs_  = 0;
    uint64_t swayEndMs_    = 0;
};

}  // namespace sdl3cpp::services::impl
