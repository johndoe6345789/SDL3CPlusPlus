#include "services/interfaces/workflow/rendering/workflow_postfx_setup_step.hpp"
#include "services/interfaces/workflow/rendering/postfx_samplers.hpp"

#include <SDL3/SDL_gpu.h>

#include <stdexcept>
#include <vector>

namespace sdl3cpp::services::impl {

WorkflowPostfxSetupStep::WorkflowPostfxSetupStep(
    std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)) {}

std::string WorkflowPostfxSetupStep::GetPluginId() const {
    return "postfx.setup";
}

void WorkflowPostfxSetupStep::Execute(const WorkflowStepDefinition&,
                                      WorkflowContext& context) {
    SDL_GPUDevice* device = context.Get<SDL_GPUDevice*>("gpu_device", nullptr);
    if (!device) {
        throw std::runtime_error(
            "postfx.setup: GPU device not found in context");
    }

    SDL_GPUSampler* linearSampler = CreatePostfxLinearSampler(device);
    if (!linearSampler) {
        throw std::runtime_error(
            "postfx.setup: Failed to create linear sampler");
    }
    context.Set<SDL_GPUSampler*>("postfx_linear_sampler", linearSampler);

    SDL_GPUSampler* nearestSampler = CreatePostfxNearestSampler(device);
    if (!nearestSampler) {
        throw std::runtime_error(
            "postfx.setup: Failed to create nearest sampler");
    }
    context.Set<SDL_GPUSampler*>("postfx_nearest_sampler", nearestSampler);

    context.Set<std::vector<float>>("ssao_kernel", GenerateSsaoKernel(16));
    context.Set<bool>("postfx_initialized", true);

    if (logger_) {
        logger_->Info(
            "postfx.setup: Samplers + SSAO kernel (16 samples) created");
    }
}

}  // namespace sdl3cpp::services::impl
