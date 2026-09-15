#pragma once

#include "services/interfaces/workflow_step_definition.hpp"

namespace sdl3cpp::services::impl {

/// q3.bots.update's tunable parameters, each with the original defaults.
struct BotUpdateParams {
    float chaseRange        = 20.0f;
    float shootRange        = 6.0f;
    float moveSpeed         = 3.0f;
    int legIdle             = 162;  // Q3 keel model defaults
    int legRun              = 167;
    int legRunCount         = 8;
    int torsoStand          = 101;
    int torsoAttack         = 107;
    int torsoAttackCount    = 6;
    int shootIntervalFrames = 30;
    int replanFrames        = 60;  // A* replan interval
};

BotUpdateParams ReadBotUpdateParams(const WorkflowStepDefinition& step);

}  // namespace sdl3cpp::services::impl
