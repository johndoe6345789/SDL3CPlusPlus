#pragma once

#include "services/interfaces/workflow_step_definition.hpp"

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

nlohmann::json BuildGridConfigJson(const GridSetupParams& params);

}  // namespace sdl3cpp::services::impl
