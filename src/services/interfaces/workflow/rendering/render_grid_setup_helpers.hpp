#pragma once

#include "services/interfaces/workflow_context.hpp"
#include "services/interfaces/workflow_step_definition.hpp"

#include <SDL3/SDL_gpu.h>
#include <nlohmann/json.hpp>

#include <cstdint>

namespace sdl3cpp::services::impl {

/// render.grid.setup's tunable parameters, each with the original
/// defaults.
struct GridSetupParams {
    uint32_t gridWidth = 11, gridHeight = 11;
    float gridSpacing = 3.0f;
    float gridStartX = -15.0f, gridStartY = -15.0f;
    float rotationOffsetX = 0.21f, rotationOffsetY = 0.37f;
    float bgColorR = 0.18f, bgColorG = 0.18f, bgColorB = 0.18f;
    uint32_t numFrames = 600;
};

GridSetupParams ReadGridSetupParams(const WorkflowStepDefinition& step);

/// Throws std::runtime_error naming whichever GPU resource is missing:
/// device/window, pipeline, or vertex/index buffers.
void ValidateGridSetupGpuResources(const WorkflowContext& context);

/// Creates a D32_FLOAT depth-stencil target sized to the window. Throws
/// std::runtime_error on failure.
SDL_GPUTexture* CreateGridDepthTexture(SDL_GPUDevice* device, int width,
                                       int height);

nlohmann::json BuildGridConfigJson(const GridSetupParams& params);

}  // namespace sdl3cpp::services::impl
