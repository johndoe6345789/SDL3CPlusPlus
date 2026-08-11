#include "services/interfaces/workflow/rendering/workflow_postfx_setup_step.hpp"

#include <SDL3/SDL_gpu.h>
#include <stdexcept>
#include <vector>
#include <cmath>

namespace sdl3cpp::services::impl {

WorkflowPostfxSetupStep::WorkflowPostfxSetupStep(std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)) {}

std::string WorkflowPostfxSetupStep::GetPluginId() const {
    return "postfx.setup";
}

void WorkflowPostfxSetupStep::Execute(const WorkflowStepDefinition& step, WorkflowContext& context) {
    SDL_GPUDevice* device = context.Get<SDL_GPUDevice*>("gpu_device", nullptr);
    if (!device) {
        throw std::runtime_error("postfx.setup: GPU device not found in context");
    }

    // Linear sampler for HDR / color texture sampling
    SDL_GPUSamplerCreateInfo linearInfo = {};
    linearInfo.min_filter = SDL_GPU_FILTER_LINEAR;
    linearInfo.mag_filter = SDL_GPU_FILTER_LINEAR;
    linearInfo.address_mode_u = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
    linearInfo.address_mode_v = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
    linearInfo.address_mode_w = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;

    SDL_GPUSampler* linearSampler = SDL_CreateGPUSampler(device, &linearInfo);
    if (!linearSampler) {
        throw std::runtime_error("postfx.setup: Failed to create linear sampler");
    }
    context.Set<SDL_GPUSampler*>("postfx_linear_sampler", linearSampler);

    // Nearest sampler for depth texture sampling (SSAO)
    SDL_GPUSamplerCreateInfo nearestInfo = {};
    nearestInfo.min_filter = SDL_GPU_FILTER_NEAREST;
    nearestInfo.mag_filter = SDL_GPU_FILTER_NEAREST;
    nearestInfo.address_mode_u = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
    nearestInfo.address_mode_v = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
    nearestInfo.address_mode_w = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;

    SDL_GPUSampler* nearestSampler = SDL_CreateGPUSampler(device, &nearestInfo);
    if (!nearestSampler) {
        throw std::runtime_error("postfx.setup: Failed to create nearest sampler");
    }
    context.Set<SDL_GPUSampler*>("postfx_nearest_sampler", nearestSampler);

    // Generate SSAO hemisphere kernel (16 samples, quadratic distribution)
    auto hashFloat = [](int i, int seed) -> float {
        int h = i * 374761393 + seed * 668265263;
        h = (h ^ (h >> 13)) * 1274126177;
        return float(h & 0x7FFFFFFF) / float(0x7FFFFFFF);
    };

    std::vector<float> kernel;
    kernel.reserve(16 * 4);
    for (int i = 0; i < 16; ++i) {
        float x = hashFloat(i, 0) * 2.0f - 1.0f;
        float y = hashFloat(i, 1) * 2.0f - 1.0f;
        float z = hashFloat(i, 2);  // hemisphere: z >= 0

        float len = std::sqrt(x * x + y * y + z * z);
        if (len < 0.001f) { x = 0; y = 0; z = 1; len = 1; }
        x /= len; y /= len; z /= len;

        // Quadratic scale: more samples near the surface
        float scale = float(i) / 16.0f;
        scale = 0.1f + scale * scale * 0.9f;

        kernel.push_back(x * scale);
        kernel.push_back(y * scale);
        kernel.push_back(z * scale);
        kernel.push_back(0.0f);
    }
    context.Set<std::vector<float>>("ssao_kernel", kernel);

    context.Set<bool>("postfx_initialized", true);

    if (logger_) {
        logger_->Info("postfx.setup: Samplers + SSAO kernel (16 samples) created");
    }
}

}  // namespace sdl3cpp::services::impl
