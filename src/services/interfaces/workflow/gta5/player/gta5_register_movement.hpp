#pragma once

#include "services/interfaces/i_logger.hpp"
#include "services/interfaces/i_workflow_step_registry.hpp"
#include "services/interfaces/workflow/gta5/stream/gta5_stream_state.hpp"

#include <memory>

namespace sdl3cpp::services::impl {

/// The ways the player moves besides the q3 walk: swimming, flying and
/// climbing.
void RegisterGta5MovementSteps(
    IWorkflowStepRegistry& registry, const std::shared_ptr<ILogger>& logger,
    const std::shared_ptr<Gta5StreamState>& state);

}  // namespace sdl3cpp::services::impl
