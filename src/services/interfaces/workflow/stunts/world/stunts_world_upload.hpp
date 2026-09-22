#pragma once

#include "services/interfaces/i_logger.hpp"
#include "services/interfaces/workflow/stunts/world/stunts_world_state.hpp"
#include "services/interfaces/workflow_context.hpp"

#include <memory>

namespace sdl3cpp::services::impl {

/**
 * @brief Meshes the loaded track and uploads both halves to the GPU.
 *
 * Sets `state.loaded` only once the road mesh is on the device, so a
 * failed upload leaves the step to try again on a later frame rather
 * than drawing empty buffers.
 */
void UploadStuntsWorld(WorkflowContext& context, StuntsWorldState& state,
                       const std::shared_ptr<ILogger>& logger);

/// Releases both meshes' buffers.
void ReleaseStuntsWorld(SDL_GPUDevice* device, StuntsWorldState& state);

}  // namespace sdl3cpp::services::impl
