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
/// Trace: why and where the pod was put back on the lap.
void TraceRacerRespawn(const std::shared_ptr<ILogger>& logger,
                       const RacerWorldState& state, const char* reason);

/// True while the pod drives itself: past the finish, or with the
/// `autopilot` parameter (RACER_AUTOPILOT) set.
bool RacerPodOnAutopilot(const WorkflowStepDefinition& step,
                         const RacerWorldState& state);

RacerPodInput ReadRacerPodInput(const WorkflowStepDefinition& step,
                                const WorkflowContext& context,
                                const RacerWorldState& state);

/// The track surface as the pod physics sees it.
RacerSurface RacerGroundSurface(const RacerGround& ground);

/// Publishes the pod's pose, speed (km/h), heat, damage and boost.
void PublishRacerPod(WorkflowContext& context, const RacerWorldState& state);

/// Trace: where the pod is, its lap point, speed and whether it is on
/// the ground. The drive step calls it once a second.
void TraceRacerPod(const std::shared_ptr<ILogger>& logger,
                   const RacerWorldState& state);

}  // namespace sdl3cpp::services::impl
