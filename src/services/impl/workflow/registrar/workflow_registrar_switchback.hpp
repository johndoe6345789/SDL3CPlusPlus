#pragma once

#include "services/interfaces/i_logger.hpp"
#include "services/interfaces/i_workflow_step_registry.hpp"
#include "services/interfaces/workflow/gta5/stream/gta5_stream_state.hpp"

#include <memory>

namespace sdl3cpp::services::impl::registrar_detail {

/// The Switchback steps: its terrain, and the car-only control that drives
/// the GTA V vehicle held in the shared gta5 vehicle state. Returns how many.
int RegisterSwitchbackSteps(std::shared_ptr<IWorkflowStepRegistry> registry,
                            std::shared_ptr<ILogger> logger,
                            std::shared_ptr<Gta5StreamState> vehicles);

}  // namespace sdl3cpp::services::impl::registrar_detail
