#pragma once

#include "services/interfaces/workflow_context.hpp"

namespace sdl3cpp::services::impl {

/// The drawn character's own walk cycle, so a step sounds as its foot
/// lands; in first person, @p phase paced as that character's gait
/// would be, @p speed m/s over @p dt seconds.
float Gta5FootPhase(const WorkflowContext& context, float phase,
                    float speed, float dt);

}  // namespace sdl3cpp::services::impl
