#pragma once

#include "services/interfaces/workflow_context.hpp"

namespace sdl3cpp::services::impl {

/**
 * @brief Seconds since the session started, as the q3 frame counts them.
 *
 * time.frame_delta, the first step of the q3 frame, publishes
 * frame.elapsed_time. The seed pipeline calls the same quantity
 * frame.elapsed and nothing sets that here, so a step reading it alone
 * sees a clock pinned at zero: items never respawn, animations never
 * advance, and timers never expire. One accessor so there is a single
 * spelling to get wrong.
 */
inline double Q3ElapsedSeconds(const WorkflowContext& context) {
    return context.GetDouble("frame.elapsed_time",
                             context.GetDouble("frame.elapsed", 0.0));
}

}  // namespace sdl3cpp::services::impl
