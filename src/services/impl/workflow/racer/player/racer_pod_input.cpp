#include "services/interfaces/workflow/racer/player/racer_pod_report.hpp"

#include "services/interfaces/workflow/racer/player/racer_autopilot.hpp"
#include "services/interfaces/workflow/racer/racer_step_params.hpp"

namespace sdl3cpp::services::impl {

RacerPodInput ReadRacerPodInput(const WorkflowStepDefinition& step,
                                const WorkflowContext& context,
                                const RacerWorldState& state) {
    RacerPodInput input;
    if (state.race.countdown <= 0.f && !state.race.finished) {
        input.throttle = context.Get<float>("input.move_forward", 0.f);
        input.steer = context.Get<float>("input.move_right", 0.f);
        input.boost = context.GetBool("racer.boost_pressed", false);
        input.repair = context.GetBool("racer.repair_pressed", false);
        // `autopilot` is "${env:RACER_AUTOPILOT}": any value but empty
        // or 0 lets the pod drive itself.
        const std::string autopilot =
            RacerStringParam(step, "autopilot", nullptr, "");
        if (!autopilot.empty() && autopilot != "0") {
            input = RacerAutopilot(state.pod, state.lapPoints,
                                   state.race.segment);
        }
    }
    return input;
}

}  // namespace sdl3cpp::services::impl
