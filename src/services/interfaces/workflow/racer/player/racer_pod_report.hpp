#pragma once

#include "services/interfaces/i_logger.hpp"
#include "services/interfaces/workflow/racer/world/racer_world_state.hpp"
#include "services/interfaces/workflow_context.hpp"
#include "services/interfaces/workflow_step_definition.hpp"

#include <memory>

namespace sdl3cpp::services::impl {

/// This frame's controls: the player's (input.move_forward,
/// input.move_right, racer.boost_pressed, racer.repair_pressed), the
/// autopilot's when the step's `autopilot` parameter is set and not "0",
/// or none during the countdown and after the finish.
RacerPodInput ReadRacerPodInput(const WorkflowStepDefinition& step,
                                const WorkflowContext& context,
                                const RacerWorldState& state);

/// Publishes the pod's pose, speed (km/h), heat, damage and boost.
void PublishRacerPod(WorkflowContext& context, const RacerWorldState& state);

/// Trace: where the pod is, its lap point, speed and whether it is on
/// the ground. The drive step calls it once a second.
void TraceRacerPod(const std::shared_ptr<ILogger>& logger,
                   const RacerWorldState& state);

}  // namespace sdl3cpp::services::impl
