#pragma once

#include "services/interfaces/i_logger.hpp"
#include "services/interfaces/i_workflow_step_registry.hpp"
#include "services/interfaces/workflow/gta5/gta5_stream_state.hpp"

#include <memory>

namespace sdl3cpp::services::impl::registrar_detail {

/// The GTA V steps around the player and the passes: flying, the
/// third-person camera, the character, the shadow map, water, mirror.
/// They share the streaming state with the rest. Returns how many.
int RegisterGta5PlayerSteps(std::shared_ptr<IWorkflowStepRegistry> registry,
                            std::shared_ptr<ILogger> logger,
                            std::shared_ptr<Gta5StreamState> state);

}  // namespace sdl3cpp::services::impl::registrar_detail
