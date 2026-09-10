#pragma once

#include "services/interfaces/workflow_step_definition.hpp"

#include <SDL3/SDL_video.h>

#include <string>

namespace sdl3cpp::services::impl {

class WorkflowStepIoResolver;

/// False for a minimized/zero-size window, which can't be blitted from.
bool HasValidWindowSize(SDL_Window* window);

/// graphics.framebuffer.readback's input/output context key names.
struct FramebufferReadbackKeys {
    std::string sourceTextureKeyKey;
    std::string outputDataKey;
    std::string outputWidthKey;
    std::string outputHeightKey;
    std::string outputSuccessKey;
};

FramebufferReadbackKeys ResolveFramebufferReadbackKeys(
    WorkflowStepIoResolver& resolver, const WorkflowStepDefinition& step);

}  // namespace sdl3cpp::services::impl
