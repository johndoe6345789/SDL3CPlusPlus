#pragma once

#include "services/interfaces/workflow_step_definition.hpp"

namespace sdl3cpp::services::impl {

/// frame.gpu.begin/frame.gpu.begin_offscreen's shared clear color, 0..1
/// per channel.
struct ClearColorParams {
    float r = 0.1f, g = 0.1f, b = 0.15f;
};

ClearColorParams ReadClearColorParams(const WorkflowStepDefinition& step);

}  // namespace sdl3cpp::services::impl
