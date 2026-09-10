#include "services/interfaces/workflow/graphics/gpu_readback_keys.hpp"
#include "services/interfaces/workflow/workflow_step_io_resolver.hpp"

#include <SDL3/SDL.h>

namespace sdl3cpp::services::impl {

bool HasValidWindowSize(SDL_Window* window) {
    int width = 0, height = 0;
    SDL_GetWindowSize(window, &width, &height);
    return width > 0 && height > 0;
}

FramebufferReadbackKeys ResolveFramebufferReadbackKeys(
    WorkflowStepIoResolver& resolver, const WorkflowStepDefinition& step) {
    FramebufferReadbackKeys keys;
    keys.sourceTextureKeyKey =
        resolver.GetRequiredInputKey(step, "source_texture_key");
    keys.outputDataKey   = resolver.GetRequiredOutputKey(step, "output_key");
    keys.outputWidthKey  = resolver.GetRequiredOutputKey(step, "output_width");
    keys.outputHeightKey = resolver.GetRequiredOutputKey(step, "output_height");
    keys.outputSuccessKey = resolver.GetRequiredOutputKey(step, "success");
    return keys;
}

}  // namespace sdl3cpp::services::impl
