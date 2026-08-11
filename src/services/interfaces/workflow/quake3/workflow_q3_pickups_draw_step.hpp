#pragma once

#include "services/interfaces/i_workflow_step.hpp"
#include "services/interfaces/i_logger.hpp"

#include <SDL3/SDL_gpu.h>
#include <memory>

namespace sdl3cpp::services::impl {

class WorkflowQ3PickupsDrawStep final : public IWorkflowStep {
public:
    explicit WorkflowQ3PickupsDrawStep(std::shared_ptr<ILogger> logger);
    ~WorkflowQ3PickupsDrawStep();

    std::string GetPluginId() const override;
    void Execute(const WorkflowStepDefinition& step, WorkflowContext& context) override;

private:
    SDL_GPUTexture* CreateColorTexture(SDL_GPUDevice* device, WorkflowContext& context,
                                       const std::string& key, uint8_t r, uint8_t g, uint8_t b);
    void EnsureBuffers(SDL_GPUDevice* device);

    std::shared_ptr<ILogger> logger_;
    SDL_GPUDevice* device_ = nullptr;
    SDL_GPUBuffer* quad_vb_ = nullptr;
    SDL_GPUBuffer* quad_ib_ = nullptr;
    SDL_GPUTransferBuffer* transfer_ = nullptr;
};

}  // namespace sdl3cpp::services::impl
